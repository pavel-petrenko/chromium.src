// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_impl_io_data.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/prefs/pref_member.h"
#include "base/prefs/pref_service.h"
#include "base/sequenced_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/worker_pool.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/net/chrome_net_log.h"
#include "chrome/browser/net/chrome_network_delegate.h"
#include "chrome/browser/net/connect_interceptor.h"
#include "chrome/browser/net/cookie_store_util.h"
#include "chrome/browser/net/http_server_properties_manager.h"
#include "chrome/browser/net/predictor.h"
#include "chrome/browser/net/sqlite_server_bound_cert_store.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/domain_reliability/monitor.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/browser/extension_protocols.h"
#include "extensions/common/constants.h"
#include "net/base/cache_type.h"
#include "net/ftp/ftp_network_layer.h"
#include "net/http/http_cache.h"
#include "net/ssl/server_bound_cert_service.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "webkit/browser/quota/special_storage_policy.h"

#if defined(OS_ANDROID) || defined(OS_IOS)
#if defined(SPDY_PROXY_AUTH_VALUE)
#include "components/data_reduction_proxy/browser/data_reduction_proxy_settings.h"
#endif
#endif

namespace {

// Identifies Chrome as the source of Domain Reliability uploads it sends.
const char* kDomainReliabilityUploadReporterString = "chrome";

net::BackendType ChooseCacheBackendType() {
#if defined(OS_ANDROID)
  return net::CACHE_BACKEND_SIMPLE;
#else
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kUseSimpleCacheBackend)) {
    const std::string opt_value =
        command_line.GetSwitchValueASCII(switches::kUseSimpleCacheBackend);
    if (LowerCaseEqualsASCII(opt_value, "off"))
      return net::CACHE_BACKEND_BLOCKFILE;
    if (opt_value == "" || LowerCaseEqualsASCII(opt_value, "on"))
      return net::CACHE_BACKEND_SIMPLE;
  }
  const std::string experiment_name =
      base::FieldTrialList::FindFullName("SimpleCacheTrial");
  if (experiment_name == "ExperimentYes" ||
      experiment_name == "ExperimentYes2") {
    return net::CACHE_BACKEND_SIMPLE;
  }
  return net::CACHE_BACKEND_BLOCKFILE;
#endif
}

bool IsDomainReliabilityMonitoringEnabled() {
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kDisableDomainReliability))
    return false;
  if (command_line->HasSwitch(switches::kEnableDomainReliability))
    return true;
  return base::FieldTrialList::FindFullName("DomRel-Enable") == "enable";
}

}  // namespace

using content::BrowserThread;

ProfileImplIOData::Handle::Handle(Profile* profile)
    : io_data_(new ProfileImplIOData),
      profile_(profile),
      initialized_(false) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(profile);
}

ProfileImplIOData::Handle::~Handle() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (io_data_->predictor_ != NULL) {
    // io_data_->predictor_ might be NULL if Init() was never called
    // (i.e. we shut down before ProfileImpl::DoFinalInit() got called).
    bool save_prefs = true;
#if defined(OS_CHROMEOS)
    save_prefs = !chromeos::ProfileHelper::IsSigninProfile(profile_);
#endif
    if (save_prefs) {
      io_data_->predictor_->SaveStateForNextStartupAndTrim(
          profile_->GetPrefs());
    }
    io_data_->predictor_->ShutdownOnUIThread();
  }

  if (io_data_->http_server_properties_manager_)
    io_data_->http_server_properties_manager_->ShutdownOnUIThread();
  io_data_->ShutdownOnUIThread();
}

void ProfileImplIOData::Handle::Init(
      const base::FilePath& cookie_path,
      const base::FilePath& server_bound_cert_path,
      const base::FilePath& cache_path,
      int cache_max_size,
      const base::FilePath& media_cache_path,
      int media_cache_max_size,
      const base::FilePath& extensions_cookie_path,
      const base::FilePath& profile_path,
      const base::FilePath& infinite_cache_path,
      chrome_browser_net::Predictor* predictor,
      content::CookieStoreConfig::SessionCookieMode session_cookie_mode,
      quota::SpecialStoragePolicy* special_storage_policy) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!io_data_->lazy_params_);
  DCHECK(predictor);

  LazyParams* lazy_params = new LazyParams();

  lazy_params->cookie_path = cookie_path;
  lazy_params->server_bound_cert_path = server_bound_cert_path;
  lazy_params->cache_path = cache_path;
  lazy_params->cache_max_size = cache_max_size;
  lazy_params->media_cache_path = media_cache_path;
  lazy_params->media_cache_max_size = media_cache_max_size;
  lazy_params->extensions_cookie_path = extensions_cookie_path;
  lazy_params->infinite_cache_path = infinite_cache_path;
  lazy_params->session_cookie_mode = session_cookie_mode;
  lazy_params->special_storage_policy = special_storage_policy;

  io_data_->lazy_params_.reset(lazy_params);

  // Keep track of profile path and cache sizes separately so we can use them
  // on demand when creating storage isolated URLRequestContextGetters.
  io_data_->profile_path_ = profile_path;
  io_data_->app_cache_max_size_ = cache_max_size;
  io_data_->app_media_cache_max_size_ = media_cache_max_size;

  io_data_->predictor_.reset(predictor);

  io_data_->InitializeMetricsEnabledStateOnUIThread();
}

content::ResourceContext*
    ProfileImplIOData::Handle::GetResourceContext() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  LazyInitialize();
  return GetResourceContextNoInit();
}

content::ResourceContext*
ProfileImplIOData::Handle::GetResourceContextNoInit() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Don't call LazyInitialize here, since the resource context is created at
  // the beginning of initalization and is used by some members while they're
  // being initialized (i.e. AppCacheService).
  return io_data_->GetResourceContext();
}

scoped_refptr<ChromeURLRequestContextGetter>
ProfileImplIOData::Handle::CreateMainRequestContextGetter(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors,
    PrefService* local_state,
    IOThread* io_thread) const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  LazyInitialize();
  DCHECK(!main_request_context_getter_.get());
  main_request_context_getter_ = ChromeURLRequestContextGetter::Create(
      profile_, io_data_, protocol_handlers, request_interceptors.Pass());

  io_data_->predictor_
      ->InitNetworkPredictor(profile_->GetPrefs(),
                             local_state,
                             io_thread,
                             main_request_context_getter_.get());

  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_PROFILE_URL_REQUEST_CONTEXT_GETTER_INITIALIZED,
      content::Source<Profile>(profile_),
      content::NotificationService::NoDetails());
  return main_request_context_getter_;
}

scoped_refptr<ChromeURLRequestContextGetter>
ProfileImplIOData::Handle::GetMediaRequestContextGetter() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  LazyInitialize();
  if (!media_request_context_getter_.get()) {
    media_request_context_getter_ =
        ChromeURLRequestContextGetter::CreateForMedia(profile_, io_data_);
  }
  return media_request_context_getter_;
}

scoped_refptr<ChromeURLRequestContextGetter>
ProfileImplIOData::Handle::GetExtensionsRequestContextGetter() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  LazyInitialize();
  if (!extensions_request_context_getter_.get()) {
    extensions_request_context_getter_ =
        ChromeURLRequestContextGetter::CreateForExtensions(profile_, io_data_);
  }
  return extensions_request_context_getter_;
}

scoped_refptr<ChromeURLRequestContextGetter>
ProfileImplIOData::Handle::CreateIsolatedAppRequestContextGetter(
    const base::FilePath& partition_path,
    bool in_memory,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Check that the partition_path is not the same as the base profile path. We
  // expect isolated partition, which will never go to the default profile path.
  CHECK(partition_path != profile_->GetPath());
  LazyInitialize();

  // Keep a map of request context getters, one per requested storage partition.
  StoragePartitionDescriptor descriptor(partition_path, in_memory);
  ChromeURLRequestContextGetterMap::iterator iter =
      app_request_context_getter_map_.find(descriptor);
  if (iter != app_request_context_getter_map_.end())
    return iter->second;

  scoped_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
      protocol_handler_interceptor(
          ProtocolHandlerRegistryFactory::GetForProfile(profile_)->
              CreateJobInterceptorFactory());
  ChromeURLRequestContextGetter* context =
      ChromeURLRequestContextGetter::CreateForIsolatedApp(
          profile_,
          io_data_,
          descriptor,
          protocol_handler_interceptor.Pass(),
          protocol_handlers,
          request_interceptors.Pass());
  app_request_context_getter_map_[descriptor] = context;

  return context;
}

scoped_refptr<ChromeURLRequestContextGetter>
ProfileImplIOData::Handle::GetIsolatedMediaRequestContextGetter(
    const base::FilePath& partition_path,
    bool in_memory) const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // We must have a non-default path, or this will act like the default media
  // context.
  CHECK(partition_path != profile_->GetPath());
  LazyInitialize();

  // Keep a map of request context getters, one per requested storage partition.
  StoragePartitionDescriptor descriptor(partition_path, in_memory);
  ChromeURLRequestContextGetterMap::iterator iter =
      isolated_media_request_context_getter_map_.find(descriptor);
  if (iter != isolated_media_request_context_getter_map_.end())
    return iter->second;

  // Get the app context as the starting point for the media context, so that
  // it uses the app's cookie store.
  ChromeURLRequestContextGetterMap::const_iterator app_iter =
      app_request_context_getter_map_.find(descriptor);
  DCHECK(app_iter != app_request_context_getter_map_.end());
  ChromeURLRequestContextGetter* app_context = app_iter->second.get();
  ChromeURLRequestContextGetter* context =
      ChromeURLRequestContextGetter::CreateForIsolatedMedia(
          profile_, app_context, io_data_, descriptor);
  isolated_media_request_context_getter_map_[descriptor] = context;

  return context;
}

void ProfileImplIOData::Handle::ClearNetworkingHistorySince(
    base::Time time,
    const base::Closure& completion) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  LazyInitialize();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(
          &ProfileImplIOData::ClearNetworkingHistorySinceOnIOThread,
          base::Unretained(io_data_),
          time,
          completion));
}

void ProfileImplIOData::Handle::ClearDomainReliabilityMonitor(
    domain_reliability::DomainReliabilityClearMode mode,
    const base::Closure& completion) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  LazyInitialize();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(
          &ProfileImplIOData::ClearDomainReliabilityMonitorOnIOThread,
          base::Unretained(io_data_),
          mode,
          completion));
}

void ProfileImplIOData::Handle::LazyInitialize() const {
  if (initialized_)
    return;

  // Set initialized_ to true at the beginning in case any of the objects
  // below try to get the ResourceContext pointer.
  initialized_ = true;
  PrefService* pref_service = profile_->GetPrefs();
  io_data_->http_server_properties_manager_ =
      new chrome_browser_net::HttpServerPropertiesManager(pref_service);
  io_data_->set_http_server_properties(
      scoped_ptr<net::HttpServerProperties>(
          io_data_->http_server_properties_manager_));
  io_data_->session_startup_pref()->Init(
      prefs::kRestoreOnStartup, pref_service);
  io_data_->session_startup_pref()->MoveToThread(
      BrowserThread::GetMessageLoopProxyForThread(BrowserThread::IO));
#if defined(FULL_SAFE_BROWSING) || defined(MOBILE_SAFE_BROWSING)
  io_data_->safe_browsing_enabled()->Init(prefs::kSafeBrowsingEnabled,
      pref_service);
  io_data_->safe_browsing_enabled()->MoveToThread(
      BrowserThread::GetMessageLoopProxyForThread(BrowserThread::IO));
#endif
  io_data_->InitializeOnUIThread(profile_);
}

ProfileImplIOData::LazyParams::LazyParams()
    : cache_max_size(0),
      media_cache_max_size(0),
      session_cookie_mode(
          content::CookieStoreConfig::EPHEMERAL_SESSION_COOKIES) {}

ProfileImplIOData::LazyParams::~LazyParams() {}

ProfileImplIOData::ProfileImplIOData()
    : ProfileIOData(Profile::REGULAR_PROFILE),
      http_server_properties_manager_(NULL),
      app_cache_max_size_(0),
      app_media_cache_max_size_(0) {
}

ProfileImplIOData::~ProfileImplIOData() {
  DestroyResourceContext();

  if (media_request_context_)
    media_request_context_->AssertNoURLRequests();
}

void ProfileImplIOData::InitializeInternal(
    ProfileParams* profile_params,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) const {
  ChromeURLRequestContext* main_context = main_request_context();

  IOThread* const io_thread = profile_params->io_thread;
  IOThread::Globals* const io_thread_globals = io_thread->globals();

  network_delegate()->set_predictor(predictor_.get());

  // Initialize context members.

  ApplyProfileParamsToContext(main_context);

  if (http_server_properties_manager_)
    http_server_properties_manager_->InitializeOnIOThread();

  main_context->set_transport_security_state(transport_security_state());

  main_context->set_net_log(io_thread->net_log());

  main_context->set_network_delegate(network_delegate());

  main_context->set_http_server_properties(http_server_properties());

  main_context->set_host_resolver(
      io_thread_globals->host_resolver.get());
  main_context->set_cert_transparency_verifier(
      io_thread_globals->cert_transparency_verifier.get());
  main_context->set_http_auth_handler_factory(
      io_thread_globals->http_auth_handler_factory.get());

  main_context->set_fraudulent_certificate_reporter(
      fraudulent_certificate_reporter());

  main_context->set_throttler_manager(
      io_thread_globals->throttler_manager.get());

  main_context->set_proxy_service(proxy_service());

  scoped_refptr<net::CookieStore> cookie_store = NULL;
  net::ServerBoundCertService* server_bound_cert_service = NULL;
  if (chrome_browser_net::ShouldUseInMemoryCookiesAndCache()) {
    // Don't use existing cookies and use an in-memory store.
    using content::CookieStoreConfig;
    cookie_store = content::CreateCookieStore(CookieStoreConfig(
        base::FilePath(),
        CookieStoreConfig::EPHEMERAL_SESSION_COOKIES,
        NULL,
        profile_params->cookie_monster_delegate.get()));
    // Don't use existing server-bound certs and use an in-memory store.
    server_bound_cert_service = new net::ServerBoundCertService(
        new net::DefaultServerBoundCertStore(NULL),
        base::WorkerPool::GetTaskRunner(true));
  }


  // setup cookie store
  if (!cookie_store.get()) {
    DCHECK(!lazy_params_->cookie_path.empty());

    content::CookieStoreConfig cookie_config(
        lazy_params_->cookie_path,
        lazy_params_->session_cookie_mode,
        lazy_params_->special_storage_policy.get(),
        profile_params->cookie_monster_delegate.get());
    cookie_config.crypto_delegate =
      chrome_browser_net::GetCookieCryptoDelegate();
    cookie_store = content::CreateCookieStore(cookie_config);
  }

  main_context->set_cookie_store(cookie_store.get());

  // Setup server bound cert service.
  if (!server_bound_cert_service) {
    DCHECK(!lazy_params_->server_bound_cert_path.empty());

    scoped_refptr<SQLiteServerBoundCertStore> server_bound_cert_db =
        new SQLiteServerBoundCertStore(
            lazy_params_->server_bound_cert_path,
            BrowserThread::GetBlockingPool()->GetSequencedTaskRunner(
                BrowserThread::GetBlockingPool()->GetSequenceToken()),
            lazy_params_->special_storage_policy.get());
    server_bound_cert_service = new net::ServerBoundCertService(
        new net::DefaultServerBoundCertStore(server_bound_cert_db.get()),
        base::WorkerPool::GetTaskRunner(true));
  }

  set_server_bound_cert_service(server_bound_cert_service);
  main_context->set_server_bound_cert_service(server_bound_cert_service);

  net::HttpCache::DefaultBackend* main_backend =
      new net::HttpCache::DefaultBackend(
          net::DISK_CACHE,
          ChooseCacheBackendType(),
          lazy_params_->cache_path,
          lazy_params_->cache_max_size,
          BrowserThread::GetMessageLoopProxyForThread(BrowserThread::CACHE)
              .get());
  net::HttpNetworkSession::Params network_session_params;
  PopulateNetworkSessionParams(profile_params, &network_session_params);
  net::HttpCache* main_cache = new net::HttpCache(
      network_session_params, main_backend);
  main_cache->InitializeInfiniteCache(lazy_params_->infinite_cache_path);

#if defined(OS_ANDROID) || defined(OS_IOS)
#if defined(SPDY_PROXY_AUTH_VALUE)
  data_reduction_proxy::DataReductionProxySettings::
      InitDataReductionProxySession(
          main_cache->GetSession(),
          io_thread_globals->data_reduction_proxy_params.get());
#endif
#endif

  if (chrome_browser_net::ShouldUseInMemoryCookiesAndCache()) {
    main_cache->set_mode(
        chrome_browser_net::IsCookieRecordMode() ?
        net::HttpCache::RECORD : net::HttpCache::PLAYBACK);
  }

  main_http_factory_.reset(main_cache);
  main_context->set_http_transaction_factory(main_cache);

#if !defined(DISABLE_FTP_SUPPORT)
  ftp_factory_.reset(
      new net::FtpNetworkLayer(io_thread_globals->host_resolver.get()));
#endif  // !defined(DISABLE_FTP_SUPPORT)

  scoped_ptr<net::URLRequestJobFactoryImpl> main_job_factory(
      new net::URLRequestJobFactoryImpl());
  InstallProtocolHandlers(main_job_factory.get(), protocol_handlers);
  main_job_factory_ = SetUpJobFactoryDefaults(
      main_job_factory.Pass(),
      request_interceptors.Pass(),
      profile_params->protocol_handler_interceptor.Pass(),
      network_delegate(),
      ftp_factory_.get());
  main_context->set_job_factory(main_job_factory_.get());

#if defined(ENABLE_EXTENSIONS)
  InitializeExtensionsRequestContext(profile_params);
#endif

  // Create a media request context based on the main context, but using a
  // media cache.  It shares the same job factory as the main context.
  StoragePartitionDescriptor details(profile_path_, false);
  media_request_context_.reset(InitializeMediaRequestContext(main_context,
                                                             details));

  if (IsDomainReliabilityMonitoringEnabled()) {
    domain_reliability_monitor_.reset(
        new domain_reliability::DomainReliabilityMonitor(
            main_context, kDomainReliabilityUploadReporterString));
    domain_reliability_monitor_->AddBakedInConfigs();
    network_delegate()->set_domain_reliability_monitor(
        domain_reliability_monitor_.get());
  }

  lazy_params_.reset();
}

void ProfileImplIOData::
    InitializeExtensionsRequestContext(ProfileParams* profile_params) const {
  ChromeURLRequestContext* extensions_context = extensions_request_context();
  IOThread* const io_thread = profile_params->io_thread;
  IOThread::Globals* const io_thread_globals = io_thread->globals();
  ApplyProfileParamsToContext(extensions_context);

  extensions_context->set_transport_security_state(transport_security_state());

  extensions_context->set_net_log(io_thread->net_log());

  extensions_context->set_throttler_manager(
      io_thread_globals->throttler_manager.get());

  content::CookieStoreConfig cookie_config(
      lazy_params_->extensions_cookie_path,
      lazy_params_->session_cookie_mode,
      NULL, NULL);
  cookie_config.crypto_delegate =
      chrome_browser_net::GetCookieCryptoDelegate();
  net::CookieStore* extensions_cookie_store =
      content::CreateCookieStore(cookie_config);
  // Enable cookies for devtools and extension URLs.
  const char* schemes[] = {content::kChromeDevToolsScheme,
                           extensions::kExtensionScheme};
  extensions_cookie_store->GetCookieMonster()->SetCookieableSchemes(schemes, 2);
  extensions_context->set_cookie_store(extensions_cookie_store);

  scoped_ptr<net::URLRequestJobFactoryImpl> extensions_job_factory(
      new net::URLRequestJobFactoryImpl());
  // TODO(shalev): The extensions_job_factory has a NULL NetworkDelegate.
  // Without a network_delegate, this protocol handler will never
  // handle file: requests, but as a side effect it makes
  // job_factory::IsHandledProtocol return true, which prevents attempts to
  // handle the protocol externally. We pass NULL in to
  // SetUpJobFactory() to get this effect.
  extensions_job_factory_ = SetUpJobFactoryDefaults(
      extensions_job_factory.Pass(),
      content::URLRequestInterceptorScopedVector(),
      scoped_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>(),
      NULL,
      ftp_factory_.get());
  extensions_context->set_job_factory(extensions_job_factory_.get());
}

ChromeURLRequestContext* ProfileImplIOData::InitializeAppRequestContext(
    ChromeURLRequestContext* main_context,
    const StoragePartitionDescriptor& partition_descriptor,
    scoped_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
        protocol_handler_interceptor,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) const {
  // Copy most state from the main context.
  AppRequestContext* context = new AppRequestContext();
  context->CopyFrom(main_context);

  base::FilePath cookie_path = partition_descriptor.path.Append(
      chrome::kCookieFilename);
  base::FilePath cache_path =
      partition_descriptor.path.Append(chrome::kCacheDirname);

  // Use a separate HTTP disk cache for isolated apps.
  net::HttpCache::BackendFactory* app_backend = NULL;
  if (partition_descriptor.in_memory) {
    app_backend = net::HttpCache::DefaultBackend::InMemory(0);
  } else {
    app_backend = new net::HttpCache::DefaultBackend(
        net::DISK_CACHE,
        ChooseCacheBackendType(),
        cache_path,
        app_cache_max_size_,
        BrowserThread::GetMessageLoopProxyForThread(BrowserThread::CACHE)
            .get());
  }
  net::HttpNetworkSession* main_network_session =
      main_http_factory_->GetSession();
  net::HttpCache* app_http_cache =
      new net::HttpCache(main_network_session, app_backend);

  scoped_refptr<net::CookieStore> cookie_store = NULL;
  if (partition_descriptor.in_memory) {
    cookie_store = content::CreateCookieStore(content::CookieStoreConfig());
  } else if (chrome_browser_net::ShouldUseInMemoryCookiesAndCache()) {
    // Don't use existing cookies and use an in-memory store.
    // TODO(creis): We should have a cookie delegate for notifying the cookie
    // extensions API, but we need to update it to understand isolated apps
    // first.
    cookie_store = content::CreateCookieStore(content::CookieStoreConfig());
    app_http_cache->set_mode(
        chrome_browser_net::IsCookieRecordMode() ?
        net::HttpCache::RECORD : net::HttpCache::PLAYBACK);
  }

  // Use an app-specific cookie store.
  if (!cookie_store.get()) {
    DCHECK(!cookie_path.empty());

    // TODO(creis): We should have a cookie delegate for notifying the cookie
    // extensions API, but we need to update it to understand isolated apps
    // first.
    content::CookieStoreConfig cookie_config(
        cookie_path,
        content::CookieStoreConfig::EPHEMERAL_SESSION_COOKIES,
        NULL, NULL);
    cookie_config.crypto_delegate =
      chrome_browser_net::GetCookieCryptoDelegate();
    cookie_store = content::CreateCookieStore(cookie_config);
  }

  // Transfer ownership of the cookies and cache to AppRequestContext.
  context->SetCookieStore(cookie_store.get());
  context->SetHttpTransactionFactory(
      scoped_ptr<net::HttpTransactionFactory>(app_http_cache));

  scoped_ptr<net::URLRequestJobFactoryImpl> job_factory(
      new net::URLRequestJobFactoryImpl());
  InstallProtocolHandlers(job_factory.get(), protocol_handlers);
  scoped_ptr<net::URLRequestJobFactory> top_job_factory(
      SetUpJobFactoryDefaults(job_factory.Pass(),
                              request_interceptors.Pass(),
                              protocol_handler_interceptor.Pass(),
                              network_delegate(),
                              ftp_factory_.get()));
  context->SetJobFactory(top_job_factory.Pass());

  return context;
}

ChromeURLRequestContext*
ProfileImplIOData::InitializeMediaRequestContext(
    ChromeURLRequestContext* original_context,
    const StoragePartitionDescriptor& partition_descriptor) const {
  // Copy most state from the original context.
  MediaRequestContext* context = new MediaRequestContext();
  context->CopyFrom(original_context);

  // For in-memory context, return immediately after creating the new
  // context before attaching a separate cache. It is important to return
  // a new context rather than just reusing |original_context| because
  // the caller expects to take ownership of the pointer.
  if (partition_descriptor.in_memory)
    return context;

  using content::StoragePartition;
  base::FilePath cache_path;
  int cache_max_size = app_media_cache_max_size_;
  if (partition_descriptor.path == profile_path_) {
    // lazy_params_ is only valid for the default media context creation.
    cache_path = lazy_params_->media_cache_path;
    cache_max_size = lazy_params_->media_cache_max_size;
  } else {
    cache_path = partition_descriptor.path.Append(chrome::kMediaCacheDirname);
  }

  // Use a separate HTTP disk cache for isolated apps.
  net::HttpCache::BackendFactory* media_backend =
      new net::HttpCache::DefaultBackend(
          net::MEDIA_CACHE,
          ChooseCacheBackendType(),
          cache_path,
          cache_max_size,
          BrowserThread::GetMessageLoopProxyForThread(BrowserThread::CACHE)
              .get());
  net::HttpNetworkSession* main_network_session =
      main_http_factory_->GetSession();
  scoped_ptr<net::HttpTransactionFactory> media_http_cache(
      new net::HttpCache(main_network_session, media_backend));

  // Transfer ownership of the cache to MediaRequestContext.
  context->SetHttpTransactionFactory(media_http_cache.Pass());

  // Note that we do not create a new URLRequestJobFactory because
  // the media context should behave exactly like its parent context
  // in all respects except for cache behavior on media subresources.
  // The CopyFrom() step above means that our media context will use
  // the same URLRequestJobFactory instance that our parent context does.

  return context;
}

ChromeURLRequestContext*
ProfileImplIOData::AcquireMediaRequestContext() const {
  DCHECK(media_request_context_);
  return media_request_context_.get();
}

ChromeURLRequestContext* ProfileImplIOData::AcquireIsolatedAppRequestContext(
    ChromeURLRequestContext* main_context,
    const StoragePartitionDescriptor& partition_descriptor,
    scoped_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
        protocol_handler_interceptor,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) const {
  // We create per-app contexts on demand, unlike the others above.
  ChromeURLRequestContext* app_request_context =
      InitializeAppRequestContext(main_context,
                                  partition_descriptor,
                                  protocol_handler_interceptor.Pass(),
                                  protocol_handlers,
                                  request_interceptors.Pass());
  DCHECK(app_request_context);
  return app_request_context;
}

ChromeURLRequestContext*
ProfileImplIOData::AcquireIsolatedMediaRequestContext(
    ChromeURLRequestContext* app_context,
    const StoragePartitionDescriptor& partition_descriptor) const {
  // We create per-app media contexts on demand, unlike the others above.
  ChromeURLRequestContext* media_request_context =
      InitializeMediaRequestContext(app_context, partition_descriptor);
  DCHECK(media_request_context);
  return media_request_context;
}

void ProfileImplIOData::ClearNetworkingHistorySinceOnIOThread(
    base::Time time,
    const base::Closure& completion) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(initialized());

  DCHECK(transport_security_state());
  // Completes synchronously.
  transport_security_state()->DeleteAllDynamicDataSince(time);
  DCHECK(http_server_properties_manager_);
  http_server_properties_manager_->Clear(completion);
}

void ProfileImplIOData::ClearDomainReliabilityMonitorOnIOThread(
    domain_reliability::DomainReliabilityClearMode mode,
    const base::Closure& completion) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(initialized());

  if (domain_reliability_monitor_)
    domain_reliability_monitor_->ClearBrowsingData(mode);

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, completion);
}
