// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(akalin): This file is basically just a unit test for
// BookmarkChangeProcessor.  Write unit tests for
// BookmarkModelAssociator separately.

#include <map>
#include <queue>
#include <stack>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/chrome_bookmark_client.h"
#include "chrome/browser/bookmarks/chrome_bookmark_client_factory.h"
#include "chrome/browser/sync/glue/bookmark_change_processor.h"
#include "chrome/browser/sync/glue/bookmark_model_associator.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/base_bookmark_model_observer.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/sync_driver/data_type_error_handler.h"
#include "components/sync_driver/data_type_error_handler_mock.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "sync/api/sync_error.h"
#include "sync/internal_api/public/change_record.h"
#include "sync/internal_api/public/read_node.h"
#include "sync/internal_api/public/read_transaction.h"
#include "sync/internal_api/public/test/test_user_share.h"
#include "sync/internal_api/public/write_node.h"
#include "sync/internal_api/public/write_transaction.h"
#include "sync/internal_api/syncapi_internal.h"
#include "sync/syncable/mutable_entry.h"  // TODO(tim): Remove. Bug 131130.
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace browser_sync {

using syncer::BaseNode;
using testing::_;
using testing::InvokeWithoutArgs;
using testing::Mock;
using testing::StrictMock;

#if defined(OS_ANDROID) || defined(OS_IOS)
static const bool kExpectMobileBookmarks = true;
#else
static const bool kExpectMobileBookmarks = false;
#endif  // defined(OS_ANDROID) || defined(OS_IOS)

namespace {

// FakeServerChange constructs a list of syncer::ChangeRecords while modifying
// the sync model, and can pass the ChangeRecord list to a
// syncer::SyncObserver (i.e., the ProfileSyncService) to test the client
// change-application behavior.
// Tests using FakeServerChange should be careful to avoid back-references,
// since FakeServerChange will send the edits in the order specified.
class FakeServerChange {
 public:
  explicit FakeServerChange(syncer::WriteTransaction* trans) : trans_(trans) {
  }

  // Pretend that the server told the syncer to add a bookmark object.
  int64 AddWithMetaInfo(const std::string& title,
                        const std::string& url,
                        const BookmarkNode::MetaInfoMap* meta_info_map,
                        bool is_folder,
                        int64 parent_id,
                        int64 predecessor_id) {
    syncer::ReadNode parent(trans_);
    EXPECT_EQ(BaseNode::INIT_OK, parent.InitByIdLookup(parent_id));
    syncer::WriteNode node(trans_);
    if (predecessor_id == 0) {
      EXPECT_TRUE(node.InitBookmarkByCreation(parent, NULL));
    } else {
      syncer::ReadNode predecessor(trans_);
      EXPECT_EQ(BaseNode::INIT_OK, predecessor.InitByIdLookup(predecessor_id));
      EXPECT_EQ(predecessor.GetParentId(), parent.GetId());
      EXPECT_TRUE(node.InitBookmarkByCreation(parent, &predecessor));
    }
    EXPECT_EQ(node.GetPredecessorId(), predecessor_id);
    EXPECT_EQ(node.GetParentId(), parent_id);
    node.SetIsFolder(is_folder);
    node.SetTitle(title);

    sync_pb::BookmarkSpecifics specifics(node.GetBookmarkSpecifics());
    if (!is_folder)
      specifics.set_url(url);
    if (meta_info_map)
      SetNodeMetaInfo(*meta_info_map, &specifics);
    node.SetBookmarkSpecifics(specifics);

    syncer::ChangeRecord record;
    record.action = syncer::ChangeRecord::ACTION_ADD;
    record.id = node.GetId();
    changes_.push_back(record);
    return node.GetId();
  }

  int64 Add(const std::string& title,
            const std::string& url,
            bool is_folder,
            int64 parent_id,
            int64 predecessor_id) {
    return AddWithMetaInfo(title, url, NULL, is_folder, parent_id,
                           predecessor_id);
  }

  // Add a bookmark folder.
  int64 AddFolder(const std::string& title,
                  int64 parent_id,
                  int64 predecessor_id) {
    return Add(title, std::string(), true, parent_id, predecessor_id);
  }
  int64 AddFolderWithMetaInfo(const std::string& title,
                              const BookmarkNode::MetaInfoMap* meta_info_map,
                              int64 parent_id,
                              int64 predecessor_id) {
    return AddWithMetaInfo(title, std::string(), meta_info_map, true, parent_id,
                           predecessor_id);
  }

  // Add a bookmark.
  int64 AddURL(const std::string& title,
               const std::string& url,
               int64 parent_id,
               int64 predecessor_id) {
    return Add(title, url, false, parent_id, predecessor_id);
  }
  int64 AddURLWithMetaInfo(const std::string& title,
                           const std::string& url,
                           const BookmarkNode::MetaInfoMap* meta_info_map,
                           int64 parent_id,
                           int64 predecessor_id) {
    return AddWithMetaInfo(title, url, meta_info_map, false, parent_id,
                           predecessor_id);
  }

  // Pretend that the server told the syncer to delete an object.
  void Delete(int64 id) {
    {
      // Delete the sync node.
      syncer::WriteNode node(trans_);
      EXPECT_EQ(BaseNode::INIT_OK, node.InitByIdLookup(id));
      if (node.GetIsFolder())
        EXPECT_FALSE(node.GetFirstChildId());
      node.GetMutableEntryForTest()->PutServerIsDel(true);
      node.Tombstone();
    }
    {
      // Verify the deletion.
      syncer::ReadNode node(trans_);
      EXPECT_EQ(BaseNode::INIT_FAILED_ENTRY_IS_DEL, node.InitByIdLookup(id));
    }

    syncer::ChangeRecord record;
    record.action = syncer::ChangeRecord::ACTION_DELETE;
    record.id = id;
    // Deletions are always first in the changelist, but we can't actually do
    // WriteNode::Remove() on the node until its children are moved. So, as
    // a practical matter, users of FakeServerChange must move or delete
    // children before parents.  Thus, we must insert the deletion record
    // at the front of the vector.
    changes_.insert(changes_.begin(), record);
  }

  // Set a new title value, and return the old value.
  std::string ModifyTitle(int64 id, const std::string& new_title) {
    syncer::WriteNode node(trans_);
    EXPECT_EQ(BaseNode::INIT_OK, node.InitByIdLookup(id));
    std::string old_title = node.GetTitle();
    node.SetTitle(new_title);
    SetModified(id);
    return old_title;
  }

  // Set a new parent and predecessor value.  Return the old parent id.
  // We could return the old predecessor id, but it turns out not to be
  // very useful for assertions.
  int64 ModifyPosition(int64 id, int64 parent_id, int64 predecessor_id) {
    syncer::ReadNode parent(trans_);
    EXPECT_EQ(BaseNode::INIT_OK, parent.InitByIdLookup(parent_id));
    syncer::WriteNode node(trans_);
    EXPECT_EQ(BaseNode::INIT_OK, node.InitByIdLookup(id));
    int64 old_parent_id = node.GetParentId();
    if (predecessor_id == 0) {
      EXPECT_TRUE(node.SetPosition(parent, NULL));
    } else {
      syncer::ReadNode predecessor(trans_);
      EXPECT_EQ(BaseNode::INIT_OK, predecessor.InitByIdLookup(predecessor_id));
      EXPECT_EQ(predecessor.GetParentId(), parent.GetId());
      EXPECT_TRUE(node.SetPosition(parent, &predecessor));
    }
    SetModified(id);
    return old_parent_id;
  }

  void ModifyCreationTime(int64 id, int64 creation_time_us) {
    syncer::WriteNode node(trans_);
    ASSERT_EQ(BaseNode::INIT_OK, node.InitByIdLookup(id));
    sync_pb::BookmarkSpecifics specifics = node.GetBookmarkSpecifics();
    specifics.set_creation_time_us(creation_time_us);
    node.SetBookmarkSpecifics(specifics);
    SetModified(id);
  }

  void ModifyMetaInfo(int64 id,
                      const BookmarkNode::MetaInfoMap& meta_info_map) {
    syncer::WriteNode node(trans_);
    ASSERT_EQ(BaseNode::INIT_OK, node.InitByIdLookup(id));
    sync_pb::BookmarkSpecifics specifics = node.GetBookmarkSpecifics();
    SetNodeMetaInfo(meta_info_map, &specifics);
    node.SetBookmarkSpecifics(specifics);
    SetModified(id);
  }

  // Pass the fake change list to |service|.
  void ApplyPendingChanges(sync_driver::ChangeProcessor* processor) {
    processor->ApplyChangesFromSyncModel(
        trans_, 0, syncer::ImmutableChangeRecordList(&changes_));
  }

  const syncer::ChangeRecordList& changes() {
    return changes_;
  }

 private:
  // Helper function to push an ACTION_UPDATE record onto the back
  // of the changelist.
  void SetModified(int64 id) {
    // Coalesce multi-property edits.
    if (!changes_.empty() && changes_.back().id == id &&
        changes_.back().action ==
        syncer::ChangeRecord::ACTION_UPDATE)
      return;
    syncer::ChangeRecord record;
    record.action = syncer::ChangeRecord::ACTION_UPDATE;
    record.id = id;
    changes_.push_back(record);
  }

  void SetNodeMetaInfo(const BookmarkNode::MetaInfoMap& meta_info_map,
                       sync_pb::BookmarkSpecifics* specifics) {
    specifics->clear_meta_info();
    for (BookmarkNode::MetaInfoMap::const_iterator it =
        meta_info_map.begin(); it != meta_info_map.end(); ++it) {
      sync_pb::MetaInfo* meta_info = specifics->add_meta_info();
      meta_info->set_key(it->first);
      meta_info->set_value(it->second);
    }
  }


  // The transaction on which everything happens.
  syncer::WriteTransaction *trans_;

  // The change list we construct.
  syncer::ChangeRecordList changes_;
};

class ExtensiveChangesBookmarkModelObserver : public BaseBookmarkModelObserver {
 public:
  explicit ExtensiveChangesBookmarkModelObserver()
      : started_count_(0),
        completed_count_at_started_(0),
        completed_count_(0) {}

  virtual void ExtensiveBookmarkChangesBeginning(
      BookmarkModel* model) OVERRIDE {
    ++started_count_;
    completed_count_at_started_ = completed_count_;
  }

  virtual void ExtensiveBookmarkChangesEnded(BookmarkModel* model) OVERRIDE {
    ++completed_count_;
  }

  virtual void BookmarkModelChanged() OVERRIDE {}

  int get_started() const {
    return started_count_;
  }

  int get_completed_count_at_started() const {
    return completed_count_at_started_;
  }

  int get_completed() const {
    return completed_count_;
  }

 private:
  int started_count_;
  int completed_count_at_started_;
  int completed_count_;

  DISALLOW_COPY_AND_ASSIGN(ExtensiveChangesBookmarkModelObserver);
};


class ProfileSyncServiceBookmarkTest : public testing::Test {
 protected:
  enum LoadOption { LOAD_FROM_STORAGE, DELETE_EXISTING_STORAGE };
  enum SaveOption { SAVE_TO_STORAGE, DONT_SAVE_TO_STORAGE };

  ProfileSyncServiceBookmarkTest()
      : model_(NULL),
        thread_bundle_(content::TestBrowserThreadBundle::DEFAULT),
        local_merge_result_(syncer::BOOKMARKS),
        syncer_merge_result_(syncer::BOOKMARKS) {}

  virtual ~ProfileSyncServiceBookmarkTest() {
    StopSync();
    UnloadBookmarkModel();
  }

  virtual void SetUp() {
    test_user_share_.SetUp();
  }

  virtual void TearDown() {
    test_user_share_.TearDown();
  }

  bool CanSyncNode(const BookmarkNode* node) {
    return model_->client()->CanSyncNode(node);
  }

  // Inserts a folder directly to the share.
  // Do not use this after model association is complete.
  //
  // This function differs from the AddFolder() function declared elsewhere in
  // this file in that it only affects the sync model.  It would be invalid to
  // change the sync model directly after ModelAssociation.  This function can
  // be invoked prior to model association to set up first-time sync model
  // association scenarios.
  int64 AddFolderToShare(syncer::WriteTransaction* trans, std::string title) {
    EXPECT_FALSE(model_associator_);

    // Be sure to call CreatePermanentBookmarkNodes(), otherwise this will fail.
    syncer::ReadNode bookmark_bar(trans);
    EXPECT_EQ(BaseNode::INIT_OK,
              bookmark_bar.InitByTagLookupForBookmarks("bookmark_bar"));

    syncer::WriteNode node(trans);
    EXPECT_TRUE(node.InitBookmarkByCreation(bookmark_bar, NULL));
    node.SetIsFolder(true);
    node.SetTitle(title);

    return node.GetId();
  }

  // Inserts a bookmark directly to the share.
  // Do not use this after model association is complete.
  //
  // This function differs from the AddURL() function declared elsewhere in this
  // file in that it only affects the sync model.  It would be invalid to change
  // the sync model directly after ModelAssociation.  This function can be
  // invoked prior to model association to set up first-time sync model
  // association scenarios.
  int64 AddBookmarkToShare(syncer::WriteTransaction *trans,
                           int64 parent_id,
                           std::string title) {
    EXPECT_FALSE(model_associator_);

    syncer::ReadNode parent(trans);
    EXPECT_EQ(BaseNode::INIT_OK, parent.InitByIdLookup(parent_id));

    sync_pb::BookmarkSpecifics specifics;
    specifics.set_url("http://www.google.com/search?q=" + title);
    specifics.set_title(title);

    syncer::WriteNode node(trans);
    EXPECT_TRUE(node.InitBookmarkByCreation(parent, NULL));
    node.SetIsFolder(false);
    node.SetTitle(title);
    node.SetBookmarkSpecifics(specifics);

    return node.GetId();
  }

  // Load (or re-load) the bookmark model.  |load| controls use of the
  // bookmarks file on disk.  |save| controls whether the newly loaded
  // bookmark model will write out a bookmark file as it goes.
  void LoadBookmarkModel(LoadOption load, SaveOption save) {
    bool delete_bookmarks = load == DELETE_EXISTING_STORAGE;
    profile_.CreateBookmarkModel(delete_bookmarks);
    model_ = BookmarkModelFactory::GetForProfile(&profile_);
    test::WaitForBookmarkModelToLoad(model_);
    // This noticeably speeds up the unit tests that request it.
    if (save == DONT_SAVE_TO_STORAGE)
      model_->ClearStore();
    base::MessageLoop::current()->RunUntilIdle();
  }

  int GetSyncBookmarkCount() {
    syncer::ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    syncer::ReadNode node(&trans);
    if (node.InitTypeRoot(syncer::BOOKMARKS) != syncer::BaseNode::INIT_OK)
      return 0;
    return node.GetTotalNodeCount();
  }

  // Creates the bookmark root node and the permanent nodes if they don't
  // already exist.
  bool CreatePermanentBookmarkNodes() {
    bool root_exists = false;
    syncer::ModelType type = syncer::BOOKMARKS;
    {
      syncer::WriteTransaction trans(FROM_HERE,
                                     test_user_share_.user_share());
      syncer::ReadNode uber_root(&trans);
      uber_root.InitByRootLookup();

      syncer::ReadNode root(&trans);
      root_exists = (root.InitTypeRoot(type) == BaseNode::INIT_OK);
    }

    if (!root_exists) {
      if (!syncer::TestUserShare::CreateRoot(type,
                                             test_user_share_.user_share()))
        return false;
    }

    const int kNumPermanentNodes = 3;
    const std::string permanent_tags[kNumPermanentNodes] = {
#if defined(OS_IOS)
      "synced_bookmarks",
#endif
      "bookmark_bar",
      "other_bookmarks",
#if !defined(OS_IOS)
      "synced_bookmarks",
#endif
    };
    syncer::WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
    syncer::ReadNode root(&trans);
    EXPECT_EQ(BaseNode::INIT_OK, root.InitTypeRoot(type));

    // Loop through creating permanent nodes as necessary.
    int64 last_child_id = syncer::kInvalidId;
    for (int i = 0; i < kNumPermanentNodes; ++i) {
      // First check if the node already exists. This is for tests that involve
      // persistence and set up sync more than once.
      syncer::ReadNode lookup(&trans);
      if (lookup.InitByTagLookupForBookmarks(permanent_tags[i]) ==
          syncer::ReadNode::INIT_OK) {
        last_child_id = lookup.GetId();
        continue;
      }

      // If it doesn't exist, create the permanent node at the end of the
      // ordering.
      syncer::ReadNode predecessor_node(&trans);
      syncer::ReadNode* predecessor = NULL;
      if (last_child_id != syncer::kInvalidId) {
        EXPECT_EQ(BaseNode::INIT_OK,
                  predecessor_node.InitByIdLookup(last_child_id));
        predecessor = &predecessor_node;
      }
      syncer::WriteNode node(&trans);
      if (!node.InitBookmarkByCreation(root, predecessor))
        return false;
      node.SetIsFolder(true);
      node.GetMutableEntryForTest()->PutUniqueServerTag(permanent_tags[i]);
      node.SetTitle(permanent_tags[i]);
      node.SetExternalId(0);
      last_child_id = node.GetId();
    }
    return true;
  }

  bool AssociateModels() {
    DCHECK(!model_associator_);

    // Set up model associator.
    model_associator_.reset(new BookmarkModelAssociator(
        BookmarkModelFactory::GetForProfile(&profile_),
        &profile_,
        test_user_share_.user_share(),
        &mock_error_handler_,
        kExpectMobileBookmarks));

    local_merge_result_ = syncer::SyncMergeResult(syncer::BOOKMARKS);
    syncer_merge_result_ = syncer::SyncMergeResult(syncer::BOOKMARKS);
    int local_count_before = model_->root_node()->GetTotalNodeCount();
    int syncer_count_before = GetSyncBookmarkCount();

    syncer::SyncError error = model_associator_->AssociateModels(
        &local_merge_result_,
        &syncer_merge_result_);
    if (error.IsSet())
      return false;

    base::MessageLoop::current()->RunUntilIdle();

    // Verify the merge results were calculated properly.
    EXPECT_EQ(local_count_before,
              local_merge_result_.num_items_before_association());
    EXPECT_EQ(syncer_count_before,
              syncer_merge_result_.num_items_before_association());
    EXPECT_EQ(local_merge_result_.num_items_after_association(),
              local_merge_result_.num_items_before_association() +
                  local_merge_result_.num_items_added() -
                  local_merge_result_.num_items_deleted());
    EXPECT_EQ(syncer_merge_result_.num_items_after_association(),
              syncer_merge_result_.num_items_before_association() +
                  syncer_merge_result_.num_items_added() -
                  syncer_merge_result_.num_items_deleted());
    EXPECT_EQ(model_->root_node()->GetTotalNodeCount(),
              local_merge_result_.num_items_after_association());
    EXPECT_EQ(GetSyncBookmarkCount(),
              syncer_merge_result_.num_items_after_association());
    return true;
  }

  void StartSync() {
    test_user_share_.Reload();

    ASSERT_TRUE(CreatePermanentBookmarkNodes());
    ASSERT_TRUE(AssociateModels());

    // Set up change processor.
    change_processor_.reset(
        new BookmarkChangeProcessor(&profile_,
                                    model_associator_.get(),
                                    &mock_error_handler_));
    change_processor_->Start(test_user_share_.user_share());
  }

  void StopSync() {
    change_processor_.reset();
    if (model_associator_) {
      syncer::SyncError error = model_associator_->DisassociateModels();
      EXPECT_FALSE(error.IsSet());
    }
    model_associator_.reset();

    base::MessageLoop::current()->RunUntilIdle();

    // TODO(akalin): Actually close the database and flush it to disk
    // (and make StartSync reload from disk).  This would require
    // refactoring TestUserShare.
  }

  void UnloadBookmarkModel() {
    profile_.CreateBookmarkModel(false /* delete_bookmarks */);
    model_ = NULL;
    base::MessageLoop::current()->RunUntilIdle();
  }

  bool InitSyncNodeFromChromeNode(const BookmarkNode* bnode,
                                  syncer::BaseNode* sync_node) {
    return model_associator_->InitSyncNodeFromChromeId(bnode->id(),
                                                       sync_node);
  }

  void ExpectSyncerNodeMatching(syncer::BaseTransaction* trans,
                                const BookmarkNode* bnode) {
    std::string truncated_title = base::UTF16ToUTF8(bnode->GetTitle());
    syncer::SyncAPINameToServerName(truncated_title, &truncated_title);
    base::TruncateUTF8ToByteSize(truncated_title, 255, &truncated_title);
    syncer::ServerNameToSyncAPIName(truncated_title, &truncated_title);

    syncer::ReadNode gnode(trans);
    ASSERT_TRUE(InitSyncNodeFromChromeNode(bnode, &gnode));
    // Non-root node titles and parents must match.
    if (!model_->is_permanent_node(bnode)) {
      EXPECT_EQ(truncated_title, gnode.GetTitle());
      EXPECT_EQ(
          model_associator_->GetChromeNodeFromSyncId(gnode.GetParentId()),
          bnode->parent());
    }
    EXPECT_EQ(bnode->is_folder(), gnode.GetIsFolder());
    if (bnode->is_url())
      EXPECT_EQ(bnode->url(), GURL(gnode.GetBookmarkSpecifics().url()));

    // Check that meta info matches.
    const BookmarkNode::MetaInfoMap* meta_info_map = bnode->GetMetaInfoMap();
    sync_pb::BookmarkSpecifics specifics = gnode.GetBookmarkSpecifics();
    if (!meta_info_map) {
      EXPECT_EQ(0, specifics.meta_info_size());
    } else {
      EXPECT_EQ(meta_info_map->size(),
                static_cast<size_t>(specifics.meta_info_size()));
      for (int i = 0; i < specifics.meta_info_size(); i++) {
        BookmarkNode::MetaInfoMap::const_iterator it =
            meta_info_map->find(specifics.meta_info(i).key());
        EXPECT_TRUE(it != meta_info_map->end());
        EXPECT_EQ(it->second, specifics.meta_info(i).value());
      }
    }

    // Check for position matches.
    int browser_index = bnode->parent()->GetIndexOf(bnode);
    if (browser_index == 0) {
      EXPECT_EQ(gnode.GetPredecessorId(), 0);
    } else {
      const BookmarkNode* bprev =
          bnode->parent()->GetChild(browser_index - 1);
      syncer::ReadNode gprev(trans);
      ASSERT_TRUE(InitSyncNodeFromChromeNode(bprev, &gprev));
      EXPECT_EQ(gnode.GetPredecessorId(), gprev.GetId());
      EXPECT_EQ(gnode.GetParentId(), gprev.GetParentId());
    }
    // Note: the managed node is the last child of the root_node but isn't
    // synced; if CanSyncNode() is false then there is no next node to sync.
    const BookmarkNode* bnext = NULL;
    if (browser_index + 1 < bnode->parent()->child_count())
        bnext = bnode->parent()->GetChild(browser_index + 1);
    if (!bnext || !CanSyncNode(bnext)) {
      EXPECT_EQ(gnode.GetSuccessorId(), 0);
    } else {
      syncer::ReadNode gnext(trans);
      ASSERT_TRUE(InitSyncNodeFromChromeNode(bnext, &gnext));
      EXPECT_EQ(gnode.GetSuccessorId(), gnext.GetId());
      EXPECT_EQ(gnode.GetParentId(), gnext.GetParentId());
    }
    if (!bnode->empty())
      EXPECT_TRUE(gnode.GetFirstChildId());
  }

  void ExpectSyncerNodeMatching(const BookmarkNode* bnode) {
    syncer::ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    ExpectSyncerNodeMatching(&trans, bnode);
  }

  void ExpectBrowserNodeMatching(syncer::BaseTransaction* trans,
                                 int64 sync_id) {
    EXPECT_TRUE(sync_id);
    const BookmarkNode* bnode =
        model_associator_->GetChromeNodeFromSyncId(sync_id);
    ASSERT_TRUE(bnode);
    ASSERT_TRUE(CanSyncNode(bnode));

    int64 id = model_associator_->GetSyncIdFromChromeId(bnode->id());
    EXPECT_EQ(id, sync_id);
    ExpectSyncerNodeMatching(trans, bnode);
  }

  void ExpectBrowserNodeUnknown(int64 sync_id) {
    EXPECT_FALSE(model_associator_->GetChromeNodeFromSyncId(sync_id));
  }

  void ExpectBrowserNodeKnown(int64 sync_id) {
    EXPECT_TRUE(model_associator_->GetChromeNodeFromSyncId(sync_id));
  }

  void ExpectSyncerNodeKnown(const BookmarkNode* node) {
    int64 sync_id = model_associator_->GetSyncIdFromChromeId(node->id());
    EXPECT_NE(sync_id, syncer::kInvalidId);
  }

  void ExpectSyncerNodeUnknown(const BookmarkNode* node) {
    int64 sync_id = model_associator_->GetSyncIdFromChromeId(node->id());
    EXPECT_EQ(sync_id, syncer::kInvalidId);
  }

  void ExpectBrowserNodeTitle(int64 sync_id, const std::string& title) {
    const BookmarkNode* bnode =
        model_associator_->GetChromeNodeFromSyncId(sync_id);
    ASSERT_TRUE(bnode);
    EXPECT_EQ(bnode->GetTitle(), base::UTF8ToUTF16(title));
  }

  void ExpectBrowserNodeURL(int64 sync_id, const std::string& url) {
    const BookmarkNode* bnode =
        model_associator_->GetChromeNodeFromSyncId(sync_id);
    ASSERT_TRUE(bnode);
    EXPECT_EQ(GURL(url), bnode->url());
  }

  void ExpectBrowserNodeParent(int64 sync_id, int64 parent_sync_id) {
    const BookmarkNode* node =
        model_associator_->GetChromeNodeFromSyncId(sync_id);
    ASSERT_TRUE(node);
    const BookmarkNode* parent =
        model_associator_->GetChromeNodeFromSyncId(parent_sync_id);
    EXPECT_TRUE(parent);
    EXPECT_EQ(node->parent(), parent);
  }

  void ExpectModelMatch(syncer::BaseTransaction* trans) {
    const BookmarkNode* root = model_->root_node();
#if defined(OS_IOS)
    EXPECT_EQ(root->GetIndexOf(model_->mobile_node()), 0);
    EXPECT_EQ(root->GetIndexOf(model_->bookmark_bar_node()), 1);
    EXPECT_EQ(root->GetIndexOf(model_->other_node()), 2);
#else
    EXPECT_EQ(root->GetIndexOf(model_->bookmark_bar_node()), 0);
    EXPECT_EQ(root->GetIndexOf(model_->other_node()), 1);
    EXPECT_EQ(root->GetIndexOf(model_->mobile_node()), 2);
#endif

    std::stack<int64> stack;
    stack.push(bookmark_bar_id());
    while (!stack.empty()) {
      int64 id = stack.top();
      stack.pop();
      if (!id) continue;

      ExpectBrowserNodeMatching(trans, id);

      syncer::ReadNode gnode(trans);
      ASSERT_EQ(BaseNode::INIT_OK, gnode.InitByIdLookup(id));
      stack.push(gnode.GetSuccessorId());
      if (gnode.GetIsFolder())
        stack.push(gnode.GetFirstChildId());
    }
  }

  void ExpectModelMatch() {
    syncer::ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    ExpectModelMatch(&trans);
  }

  int64 mobile_bookmarks_id() {
    return
        model_associator_->GetSyncIdFromChromeId(model_->mobile_node()->id());
  }

  int64 other_bookmarks_id() {
    return
        model_associator_->GetSyncIdFromChromeId(model_->other_node()->id());
  }

  int64 bookmark_bar_id() {
    return model_associator_->GetSyncIdFromChromeId(
        model_->bookmark_bar_node()->id());
  }

 protected:
  TestingProfile profile_;
  BookmarkModel* model_;
  syncer::TestUserShare test_user_share_;
  scoped_ptr<BookmarkChangeProcessor> change_processor_;
  StrictMock<sync_driver::DataTypeErrorHandlerMock> mock_error_handler_;
  scoped_ptr<BookmarkModelAssociator> model_associator_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  syncer::SyncMergeResult local_merge_result_;
  syncer::SyncMergeResult syncer_merge_result_;
};

TEST_F(ProfileSyncServiceBookmarkTest, InitialState) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();

  EXPECT_TRUE(other_bookmarks_id());
  EXPECT_TRUE(bookmark_bar_id());
  EXPECT_TRUE(mobile_bookmarks_id());

  ExpectModelMatch();
}

// Populate the sync database then start model association.  Sync's bookmarks
// should end up being copied into the native model, resulting in a successful
// "ExpectModelMatch()".
//
// This code has some use for verifying correctness.  It's also a very useful
// for profiling bookmark ModelAssociation, an important part of some first-time
// sync scenarios.  Simply increase the kNumFolders and kNumBookmarksPerFolder
// as desired, then run the test under a profiler to find hot spots in the model
// association code.
TEST_F(ProfileSyncServiceBookmarkTest, InitialModelAssociate) {
  const int kNumBookmarksPerFolder = 10;
  const int kNumFolders = 10;

  CreatePermanentBookmarkNodes();

  {
    syncer::WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
    for (int i = 0; i < kNumFolders; ++i) {
      int64 folder_id = AddFolderToShare(&trans,
                                         base::StringPrintf("folder%05d", i));
      for (int j = 0; j < kNumBookmarksPerFolder; ++j) {
        AddBookmarkToShare(&trans,
                           folder_id,
                           base::StringPrintf("bookmark%05d", j));
      }
    }
  }

  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();

  ExpectModelMatch();
}


TEST_F(ProfileSyncServiceBookmarkTest, BookmarkModelOperations) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();

  // Test addition.
  const BookmarkNode* folder =
      model_->AddFolder(model_->other_node(), 0, base::ASCIIToUTF16("foobar"));
  ExpectSyncerNodeMatching(folder);
  ExpectModelMatch();
  const BookmarkNode* folder2 =
      model_->AddFolder(folder, 0, base::ASCIIToUTF16("nested"));
  ExpectSyncerNodeMatching(folder2);
  ExpectModelMatch();
  const BookmarkNode* url1 = model_->AddURL(
      folder, 0, base::ASCIIToUTF16("Internets #1 Pies Site"),
      GURL("http://www.easypie.com/"));
  ExpectSyncerNodeMatching(url1);
  ExpectModelMatch();
  const BookmarkNode* url2 = model_->AddURL(
      folder, 1, base::ASCIIToUTF16("Airplanes"),
      GURL("http://www.easyjet.com/"));
  ExpectSyncerNodeMatching(url2);
  ExpectModelMatch();
  // Test addition.
  const BookmarkNode* mobile_folder =
      model_->AddFolder(model_->mobile_node(), 0, base::ASCIIToUTF16("pie"));
  ExpectSyncerNodeMatching(mobile_folder);
  ExpectModelMatch();

  // Test modification.
  model_->SetTitle(url2, base::ASCIIToUTF16("EasyJet"));
  ExpectModelMatch();
  model_->Move(url1, folder2, 0);
  ExpectModelMatch();
  model_->Move(folder2, model_->bookmark_bar_node(), 0);
  ExpectModelMatch();
  model_->SetTitle(folder2, base::ASCIIToUTF16("Not Nested"));
  ExpectModelMatch();
  model_->Move(folder, folder2, 0);
  ExpectModelMatch();
  model_->SetTitle(folder, base::ASCIIToUTF16("who's nested now?"));
  ExpectModelMatch();
  model_->Copy(url2, model_->bookmark_bar_node(), 0);
  ExpectModelMatch();
  model_->SetTitle(mobile_folder, base::ASCIIToUTF16("strawberry"));
  ExpectModelMatch();

  // Test deletion.
  // Delete a single item.
  model_->Remove(url2->parent(), url2->parent()->GetIndexOf(url2));
  ExpectModelMatch();
  // Delete an item with several children.
  model_->Remove(folder2->parent(),
                 folder2->parent()->GetIndexOf(folder2));
  ExpectModelMatch();
  model_->Remove(model_->mobile_node(), 0);
  ExpectModelMatch();
}

TEST_F(ProfileSyncServiceBookmarkTest, ServerChangeProcessing) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();

  syncer::WriteTransaction trans(FROM_HERE, test_user_share_.user_share());

  FakeServerChange adds(&trans);
  int64 f1 = adds.AddFolder("Server Folder B", bookmark_bar_id(), 0);
  int64 f2 = adds.AddFolder("Server Folder A", bookmark_bar_id(), f1);
  int64 u1 = adds.AddURL("Some old site", "ftp://nifty.andrew.cmu.edu/",
                         bookmark_bar_id(), f2);
  int64 u2 = adds.AddURL("Nifty", "ftp://nifty.andrew.cmu.edu/", f1, 0);
  // u3 is a duplicate URL
  int64 u3 = adds.AddURL("Nifty2", "ftp://nifty.andrew.cmu.edu/", f1, u2);
  // u4 is a duplicate title, different URL.
  adds.AddURL("Some old site", "http://slog.thestranger.com/",
              bookmark_bar_id(), u1);
  // u5 tests an empty-string title.
  std::string javascript_url(
      "javascript:(function(){var w=window.open(" \
      "'about:blank','gnotesWin','location=0,menubar=0," \
      "scrollbars=0,status=0,toolbar=0,width=300," \
      "height=300,resizable');});");
  adds.AddURL(std::string(), javascript_url, other_bookmarks_id(), 0);
  int64 u6 = adds.AddURL(
      "Sync1", "http://www.syncable.edu/", mobile_bookmarks_id(), 0);

  syncer::ChangeRecordList::const_iterator it;
  // The bookmark model shouldn't yet have seen any of the nodes of |adds|.
  for (it = adds.changes().begin(); it != adds.changes().end(); ++it)
    ExpectBrowserNodeUnknown(it->id);

  adds.ApplyPendingChanges(change_processor_.get());

  // Make sure the bookmark model received all of the nodes in |adds|.
  for (it = adds.changes().begin(); it != adds.changes().end(); ++it)
    ExpectBrowserNodeMatching(&trans, it->id);
  ExpectModelMatch(&trans);

  // Part two: test modifications.
  FakeServerChange mods(&trans);
  // Mess with u2, and move it into empty folder f2
  // TODO(ncarter): Determine if we allow ModifyURL ops or not.
  /* std::string u2_old_url = mods.ModifyURL(u2, "http://www.google.com"); */
  std::string u2_old_title = mods.ModifyTitle(u2, "The Google");
  int64 u2_old_parent = mods.ModifyPosition(u2, f2, 0);

  // Now move f1 after u2.
  std::string f1_old_title = mods.ModifyTitle(f1, "Server Folder C");
  int64 f1_old_parent = mods.ModifyPosition(f1, f2, u2);

  // Then add u3 after f1.
  int64 u3_old_parent = mods.ModifyPosition(u3, f2, f1);

  std::string u6_old_title = mods.ModifyTitle(u6, "Mobile Folder A");

  // Test that the property changes have not yet taken effect.
  ExpectBrowserNodeTitle(u2, u2_old_title);
  /* ExpectBrowserNodeURL(u2, u2_old_url); */
  ExpectBrowserNodeParent(u2, u2_old_parent);

  ExpectBrowserNodeTitle(f1, f1_old_title);
  ExpectBrowserNodeParent(f1, f1_old_parent);

  ExpectBrowserNodeParent(u3, u3_old_parent);

  ExpectBrowserNodeTitle(u6, u6_old_title);

  // Apply the changes.
  mods.ApplyPendingChanges(change_processor_.get());

  // Check for successful application.
  for (it = mods.changes().begin(); it != mods.changes().end(); ++it)
    ExpectBrowserNodeMatching(&trans, it->id);
  ExpectModelMatch(&trans);

  // Part 3: Test URL deletion.
  FakeServerChange dels(&trans);
  dels.Delete(u2);
  dels.Delete(u3);
  dels.Delete(u6);

  ExpectBrowserNodeKnown(u2);
  ExpectBrowserNodeKnown(u3);

  dels.ApplyPendingChanges(change_processor_.get());

  ExpectBrowserNodeUnknown(u2);
  ExpectBrowserNodeUnknown(u3);
  ExpectBrowserNodeUnknown(u6);
  ExpectModelMatch(&trans);
}

// Tests a specific case in ApplyModelChanges where we move the
// children out from under a parent, and then delete the parent
// in the same changelist.  The delete shows up first in the changelist,
// requiring the children to be moved to a temporary location.
TEST_F(ProfileSyncServiceBookmarkTest, ServerChangeRequiringFosterParent) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();

  syncer::WriteTransaction trans(FROM_HERE, test_user_share_.user_share());

  // Stress the immediate children of other_node because that's where
  // ApplyModelChanges puts a temporary foster parent node.
  std::string url("http://dev.chromium.org/");
  FakeServerChange adds(&trans);
  int64 f0 = other_bookmarks_id();                 // + other_node
  int64 f1 = adds.AddFolder("f1",      f0, 0);    //   + f1
  int64 f2 = adds.AddFolder("f2",      f1, 0);    //     + f2
  int64 u3 = adds.AddURL(   "u3", url, f2, 0);    //       + u3    NOLINT
  int64 u4 = adds.AddURL(   "u4", url, f2, u3);   //       + u4    NOLINT
  int64 u5 = adds.AddURL(   "u5", url, f1, f2);   //     + u5      NOLINT
  int64 f6 = adds.AddFolder("f6",      f1, u5);   //     + f6
  int64 u7 = adds.AddURL(   "u7", url, f0, f1);   //   + u7        NOLINT

  syncer::ChangeRecordList::const_iterator it;
  // The bookmark model shouldn't yet have seen any of the nodes of |adds|.
  for (it = adds.changes().begin(); it != adds.changes().end(); ++it)
    ExpectBrowserNodeUnknown(it->id);

  adds.ApplyPendingChanges(change_processor_.get());

  // Make sure the bookmark model received all of the nodes in |adds|.
  for (it = adds.changes().begin(); it != adds.changes().end(); ++it)
    ExpectBrowserNodeMatching(&trans, it->id);
  ExpectModelMatch(&trans);

  // We have to do the moves before the deletions, but FakeServerChange will
  // put the deletion at the front of the changelist.
  FakeServerChange ops(&trans);
  ops.ModifyPosition(f6, other_bookmarks_id(), 0);
  ops.ModifyPosition(u3, other_bookmarks_id(), f1);  // Prev == f1 is OK here.
  ops.ModifyPosition(f2, other_bookmarks_id(), u7);
  ops.ModifyPosition(u7, f2, 0);
  ops.ModifyPosition(u4, other_bookmarks_id(), f2);
  ops.ModifyPosition(u5, f6, 0);
  ops.Delete(f1);

  ops.ApplyPendingChanges(change_processor_.get());

  ExpectModelMatch(&trans);
}

// Simulate a server change record containing a valid but non-canonical URL.
TEST_F(ProfileSyncServiceBookmarkTest, ServerChangeWithNonCanonicalURL) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, SAVE_TO_STORAGE);
  StartSync();

  {
    syncer::WriteTransaction trans(FROM_HERE, test_user_share_.user_share());

    FakeServerChange adds(&trans);
    std::string url("http://dev.chromium.org");
    EXPECT_NE(GURL(url).spec(), url);
    adds.AddURL("u1", url, other_bookmarks_id(), 0);

    adds.ApplyPendingChanges(change_processor_.get());

    EXPECT_EQ(1, model_->other_node()->child_count());
    ExpectModelMatch(&trans);
  }

  // Now reboot the sync service, forcing a merge step.
  StopSync();
  LoadBookmarkModel(LOAD_FROM_STORAGE, SAVE_TO_STORAGE);
  StartSync();

  // There should still be just the one bookmark.
  EXPECT_EQ(1, model_->other_node()->child_count());
  ExpectModelMatch();
}

// Simulate a server change record containing an invalid URL (per GURL).
// TODO(ncarter): Disabled due to crashes.  Fix bug 1677563.
TEST_F(ProfileSyncServiceBookmarkTest, DISABLED_ServerChangeWithInvalidURL) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, SAVE_TO_STORAGE);
  StartSync();

  int child_count = 0;
  {
    syncer::WriteTransaction trans(FROM_HERE, test_user_share_.user_share());

    FakeServerChange adds(&trans);
    std::string url("x");
    EXPECT_FALSE(GURL(url).is_valid());
    adds.AddURL("u1", url, other_bookmarks_id(), 0);

    adds.ApplyPendingChanges(change_processor_.get());

    // We're lenient about what should happen -- the model could wind up with
    // the node or without it; but things should be consistent, and we
    // shouldn't crash.
    child_count = model_->other_node()->child_count();
    EXPECT_TRUE(child_count == 0 || child_count == 1);
    ExpectModelMatch(&trans);
  }

  // Now reboot the sync service, forcing a merge step.
  StopSync();
  LoadBookmarkModel(LOAD_FROM_STORAGE, SAVE_TO_STORAGE);
  StartSync();

  // Things ought not to have changed.
  EXPECT_EQ(model_->other_node()->child_count(), child_count);
  ExpectModelMatch();
}


// Test strings that might pose a problem if the titles ever became used as
// file names in the sync backend.
TEST_F(ProfileSyncServiceBookmarkTest, CornerCaseNames) {
  // TODO(ncarter): Bug 1570238 explains the failure of this test.
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, SAVE_TO_STORAGE);
  StartSync();

  const char* names[] = {
      // The empty string.
      "",
      // Illegal Windows filenames.
      "CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4",
      "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3",
      "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
      // Current/parent directory markers.
      ".", "..", "...",
      // Files created automatically by the Windows shell.
      "Thumbs.db", ".DS_Store",
      // Names including Win32-illegal characters, and path separators.
      "foo/bar", "foo\\bar", "foo?bar", "foo:bar", "foo|bar", "foo\"bar",
      "foo'bar", "foo<bar", "foo>bar", "foo%bar", "foo*bar", "foo]bar",
      "foo[bar",
      // A name with title > 255 characters
      "012345678901234567890123456789012345678901234567890123456789012345678901"
      "234567890123456789012345678901234567890123456789012345678901234567890123"
      "456789012345678901234567890123456789012345678901234567890123456789012345"
      "678901234567890123456789012345678901234567890123456789012345678901234567"
      "890123456789"
  };
  // Create both folders and bookmarks using each name.
  GURL url("http://www.doublemint.com");
  for (size_t i = 0; i < arraysize(names); ++i) {
    model_->AddFolder(model_->other_node(), 0, base::ASCIIToUTF16(names[i]));
    model_->AddURL(model_->other_node(), 0, base::ASCIIToUTF16(names[i]), url);
  }

  // Verify that the browser model matches the sync model.
  EXPECT_EQ(static_cast<size_t>(model_->other_node()->child_count()),
            2*arraysize(names));
  ExpectModelMatch();

  // Restart and re-associate. Verify things still match.
  StopSync();
  LoadBookmarkModel(LOAD_FROM_STORAGE, SAVE_TO_STORAGE);
  StartSync();
  EXPECT_EQ(static_cast<size_t>(model_->other_node()->child_count()),
            2*arraysize(names));
  ExpectModelMatch();
}

// Stress the internal representation of position by sparse numbers. We want
// to repeatedly bisect the range of available positions, to force the
// syncer code to renumber its ranges.  Pick a number big enough so that it
// would exhaust 32bits of room between items a couple of times.
TEST_F(ProfileSyncServiceBookmarkTest, RepeatedMiddleInsertion) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();

  static const int kTimesToInsert = 256;

  // Create two book-end nodes to insert between.
  model_->AddFolder(model_->other_node(), 0, base::ASCIIToUTF16("Alpha"));
  model_->AddFolder(model_->other_node(), 1, base::ASCIIToUTF16("Omega"));
  int count = 2;

  // Test insertion in first half of range by repeatedly inserting in second
  // position.
  for (int i = 0; i < kTimesToInsert; ++i) {
    base::string16 title =
        base::ASCIIToUTF16("Pre-insertion ") + base::IntToString16(i);
    model_->AddFolder(model_->other_node(), 1, title);
    count++;
  }

  // Test insertion in second half of range by repeatedly inserting in
  // second-to-last position.
  for (int i = 0; i < kTimesToInsert; ++i) {
    base::string16 title =
        base::ASCIIToUTF16("Post-insertion ") + base::IntToString16(i);
    model_->AddFolder(model_->other_node(), count - 1, title);
    count++;
  }

  // Verify that the browser model matches the sync model.
  EXPECT_EQ(model_->other_node()->child_count(), count);
  ExpectModelMatch();
}

// Introduce a consistency violation into the model, and see that it
// puts itself into a lame, error state.
TEST_F(ProfileSyncServiceBookmarkTest, UnrecoverableErrorSuspendsService) {
  EXPECT_CALL(mock_error_handler_,
              OnSingleDatatypeUnrecoverableError(_, _));

  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();

  // Add a node which will be the target of the consistency violation.
  const BookmarkNode* node =
      model_->AddFolder(model_->other_node(), 0, base::ASCIIToUTF16("node"));
  ExpectSyncerNodeMatching(node);

  // Now destroy the syncer node as if we were the ProfileSyncService without
  // updating the ProfileSyncService state.  This should introduce
  // inconsistency between the two models.
  {
    syncer::WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
    syncer::WriteNode sync_node(&trans);
    ASSERT_TRUE(InitSyncNodeFromChromeNode(node, &sync_node));
    sync_node.Tombstone();
  }
  // The models don't match at this point, but the ProfileSyncService
  // doesn't know it yet.
  ExpectSyncerNodeKnown(node);

  // Add a child to the inconsistent node.  This should cause detection of the
  // problem and the syncer should stop processing changes.
  model_->AddFolder(node, 0, base::ASCIIToUTF16("nested"));
}

// See what happens if we run model association when there are two exact URL
// duplicate bookmarks.  The BookmarkModelAssociator should not fall over when
// this happens.
TEST_F(ProfileSyncServiceBookmarkTest, MergeDuplicates) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, SAVE_TO_STORAGE);
  StartSync();

  model_->AddURL(model_->other_node(), 0, base::ASCIIToUTF16("Dup"),
                 GURL("http://dup.com/"));
  model_->AddURL(model_->other_node(), 0, base::ASCIIToUTF16("Dup"),
                 GURL("http://dup.com/"));

  EXPECT_EQ(2, model_->other_node()->child_count());

  // Restart the sync service to trigger model association.
  StopSync();
  StartSync();

  EXPECT_EQ(2, model_->other_node()->child_count());
  ExpectModelMatch();
}

TEST_F(ProfileSyncServiceBookmarkTest, ApplySyncDeletesFromJournal) {
  // Initialize sync model and bookmark model as:
  // URL 0
  // Folder 1
  //   |-- URL 1
  //   +-- Folder 2
  //         +-- URL 2
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, SAVE_TO_STORAGE);
  int64 u0 = 0;
  int64 f1 = 0;
  int64 u1 = 0;
  int64 f2 = 0;
  int64 u2 = 0;
  StartSync();
  int fixed_sync_bk_count = GetSyncBookmarkCount();
  {
    syncer::WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
    FakeServerChange adds(&trans);
    u0 = adds.AddURL("URL 0", "http://plus.google.com/", bookmark_bar_id(), 0);
    f1 = adds.AddFolder("Folder 1", bookmark_bar_id(), u0);
    u1 = adds.AddURL("URL 1", "http://www.google.com/", f1, 0);
    f2 = adds.AddFolder("Folder 2", f1, u1);
    u2 = adds.AddURL("URL 2", "http://mail.google.com/", f2, 0);
    adds.ApplyPendingChanges(change_processor_.get());
  }
  StopSync();

  // Reload bookmark model and disable model saving to make sync changes not
  // persisted.
  LoadBookmarkModel(LOAD_FROM_STORAGE, DONT_SAVE_TO_STORAGE);
  EXPECT_EQ(6, model_->bookmark_bar_node()->GetTotalNodeCount());
  EXPECT_EQ(fixed_sync_bk_count + 5, GetSyncBookmarkCount());
  StartSync();
  {
    // Remove all folders/bookmarks except u3 added above.
    syncer::WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
    FakeServerChange dels(&trans);
    dels.Delete(u2);
    dels.Delete(f2);
    dels.Delete(u1);
    dels.Delete(f1);
    dels.ApplyPendingChanges(change_processor_.get());
  }
  StopSync();
  // Bookmark bar itself and u0 remain.
  EXPECT_EQ(2, model_->bookmark_bar_node()->GetTotalNodeCount());

  // Reload bookmarks including ones deleted in sync model from storage.
  LoadBookmarkModel(LOAD_FROM_STORAGE, DONT_SAVE_TO_STORAGE);
  EXPECT_EQ(6, model_->bookmark_bar_node()->GetTotalNodeCount());
  // Add a bookmark under f1 when sync is off so that f1 will not be
  // deleted even when f1 matches delete journal because it's not empty.
  model_->AddURL(model_->bookmark_bar_node()->GetChild(1),
                 0, base::UTF8ToUTF16("local"), GURL("http://www.youtube.com"));
  // Sync model has fixed bookmarks nodes and u3.
  EXPECT_EQ(fixed_sync_bk_count + 1, GetSyncBookmarkCount());
  StartSync();
  // Expect 4 bookmarks after model association because u2, f2, u1 are removed
  // by delete journal, f1 is not removed by delete journal because it's
  // not empty due to www.youtube.com added above.
  EXPECT_EQ(4, model_->bookmark_bar_node()->GetTotalNodeCount());
  EXPECT_EQ(base::UTF8ToUTF16("URL 0"),
            model_->bookmark_bar_node()->GetChild(0)->GetTitle());
  EXPECT_EQ(base::UTF8ToUTF16("Folder 1"),
            model_->bookmark_bar_node()->GetChild(1)->GetTitle());
  EXPECT_EQ(base::UTF8ToUTF16("local"),
            model_->bookmark_bar_node()->GetChild(1)->GetChild(0)->GetTitle());
  StopSync();

  // Verify purging of delete journals.
  // Delete journals for u2, f2, u1 remains because they are used in last
  // association.
  EXPECT_EQ(3u, test_user_share_.GetDeleteJournalSize());
  StartSync();
  StopSync();
  // Reload again and all delete journals should be gone because none is used
  // in last association.
  ASSERT_TRUE(test_user_share_.Reload());
  EXPECT_EQ(0u, test_user_share_.GetDeleteJournalSize());
}

struct TestData {
  const char* title;
  const char* url;
};

// Map from bookmark node ID to its version.
typedef std::map<int64, int64> BookmarkNodeVersionMap;

// TODO(ncarter): Integrate the existing TestNode/PopulateNodeFromString code
// in the bookmark model unittest, to make it simpler to set up test data
// here (and reduce the amount of duplication among tests), and to reduce the
// duplication.
class ProfileSyncServiceBookmarkTestWithData
    : public ProfileSyncServiceBookmarkTest {
 public:
  ProfileSyncServiceBookmarkTestWithData();

 protected:
  // Populates or compares children of the given bookmark node from/with the
  // given test data array with the given size. |running_count| is updated as
  // urls are added. It is used to set the creation date (or test the creation
  // date for CompareWithTestData()).
  void PopulateFromTestData(const BookmarkNode* node,
                            const TestData* data,
                            int size,
                            int* running_count);
  void CompareWithTestData(const BookmarkNode* node,
                           const TestData* data,
                           int size,
                           int* running_count);

  void ExpectBookmarkModelMatchesTestData();
  void WriteTestDataToBookmarkModel();

  // Output transaction versions of |node| and nodes under it to
  // |node_versions|.
  void GetTransactionVersions(const BookmarkNode* root,
                              BookmarkNodeVersionMap* node_versions);

  // Verify transaction versions of bookmark nodes and sync nodes are equal
  // recursively. If node is in |version_expected|, versions should match
  // there, too.
  void ExpectTransactionVersionMatch(
      const BookmarkNode* node,
      const BookmarkNodeVersionMap& version_expected);

 private:
  const base::Time start_time_;

  DISALLOW_COPY_AND_ASSIGN(ProfileSyncServiceBookmarkTestWithData);
};

namespace {

// Constants for bookmark model that looks like:
// |-- Bookmark bar
// |   |-- u2, http://www.u2.com/
// |   |-- f1
// |   |   |-- f1u4, http://www.f1u4.com/
// |   |   |-- f1u2, http://www.f1u2.com/
// |   |   |-- f1u3, http://www.f1u3.com/
// |   |   +-- f1u1, http://www.f1u1.com/
// |   |-- u1, http://www.u1.com/
// |   +-- f2
// |       |-- f2u2, http://www.f2u2.com/
// |       |-- f2u4, http://www.f2u4.com/
// |       |-- f2u3, http://www.f2u3.com/
// |       +-- f2u1, http://www.f2u1.com/
// +-- Other bookmarks
// |   |-- f3
// |   |   |-- f3u4, http://www.f3u4.com/
// |   |   |-- f3u2, http://www.f3u2.com/
// |   |   |-- f3u3, http://www.f3u3.com/
// |   |   +-- f3u1, http://www.f3u1.com/
// |   |-- u4, http://www.u4.com/
// |   |-- u3, http://www.u3.com/
// |   --- f4
// |   |   |-- f4u1, http://www.f4u1.com/
// |   |   |-- f4u2, http://www.f4u2.com/
// |   |   |-- f4u3, http://www.f4u3.com/
// |   |   +-- f4u4, http://www.f4u4.com/
// |   |-- dup
// |   |   +-- dupu1, http://www.dupu1.com/
// |   +-- dup
// |   |   +-- dupu2, http://www.dupu1.com/
// |   +--   ls  , http://www.ls.com/
// |
// +-- Mobile bookmarks
//     |-- f5
//     |   |-- f5u1, http://www.f5u1.com/
//     |-- f6
//     |   |-- f6u1, http://www.f6u1.com/
//     |   |-- f6u2, http://www.f6u2.com/
//     +-- u5, http://www.u5.com/

static TestData kBookmarkBarChildren[] = {
  { "u2", "http://www.u2.com/" },
  { "f1", NULL },
  { "u1", "http://www.u1.com/" },
  { "f2", NULL },
};
static TestData kF1Children[] = {
  { "f1u4", "http://www.f1u4.com/" },
  { "f1u2", "http://www.f1u2.com/" },
  { "f1u3", "http://www.f1u3.com/" },
  { "f1u1", "http://www.f1u1.com/" },
};
static TestData kF2Children[] = {
  { "f2u2", "http://www.f2u2.com/" },
  { "f2u4", "http://www.f2u4.com/" },
  { "f2u3", "http://www.f2u3.com/" },
  { "f2u1", "http://www.f2u1.com/" },
};

static TestData kOtherBookmarkChildren[] = {
  { "f3", NULL },
  { "u4", "http://www.u4.com/" },
  { "u3", "http://www.u3.com/" },
  { "f4", NULL },
  { "dup", NULL },
  { "dup", NULL },
  { "  ls  ", "http://www.ls.com/" }
};
static TestData kF3Children[] = {
  { "f3u4", "http://www.f3u4.com/" },
  { "f3u2", "http://www.f3u2.com/" },
  { "f3u3", "http://www.f3u3.com/" },
  { "f3u1", "http://www.f3u1.com/" },
};
static TestData kF4Children[] = {
  { "f4u1", "http://www.f4u1.com/" },
  { "f4u2", "http://www.f4u2.com/" },
  { "f4u3", "http://www.f4u3.com/" },
  { "f4u4", "http://www.f4u4.com/" },
};
static TestData kDup1Children[] = {
  { "dupu1", "http://www.dupu1.com/" },
};
static TestData kDup2Children[] = {
  { "dupu2", "http://www.dupu2.com/" },
};

static TestData kMobileBookmarkChildren[] = {
  { "f5", NULL },
  { "f6", NULL },
  { "u5", "http://www.u5.com/" },
};
static TestData kF5Children[] = {
  { "f5u1", "http://www.f5u1.com/" },
  { "f5u2", "http://www.f5u2.com/" },
};
static TestData kF6Children[] = {
  { "f6u1", "http://www.f6u1.com/" },
  { "f6u2", "http://www.f6u2.com/" },
};

}  // anonymous namespace.

ProfileSyncServiceBookmarkTestWithData::
ProfileSyncServiceBookmarkTestWithData()
    : start_time_(base::Time::Now()) {
}

void ProfileSyncServiceBookmarkTestWithData::PopulateFromTestData(
    const BookmarkNode* node,
    const TestData* data,
    int size,
    int* running_count) {
  DCHECK(node);
  DCHECK(data);
  DCHECK(node->is_folder());
  for (int i = 0; i < size; ++i) {
    const TestData& item = data[i];
    if (item.url) {
      const base::Time add_time =
          start_time_ + base::TimeDelta::FromMinutes(*running_count);
      model_->AddURLWithCreationTimeAndMetaInfo(node,
                                                i,
                                                base::UTF8ToUTF16(item.title),
                                                GURL(item.url),
                                                add_time,
                                                NULL);
    } else {
      model_->AddFolder(node, i, base::UTF8ToUTF16(item.title));
    }
    (*running_count)++;
  }
}

void ProfileSyncServiceBookmarkTestWithData::CompareWithTestData(
    const BookmarkNode* node,
    const TestData* data,
    int size,
    int* running_count) {
  DCHECK(node);
  DCHECK(data);
  DCHECK(node->is_folder());
  ASSERT_EQ(size, node->child_count());
  for (int i = 0; i < size; ++i) {
    const BookmarkNode* child_node = node->GetChild(i);
    const TestData& item = data[i];
    GURL url = GURL(item.url == NULL ? "" : item.url);
    BookmarkNode test_node(url);
    test_node.SetTitle(base::UTF8ToUTF16(item.title));
    EXPECT_EQ(child_node->GetTitle(), test_node.GetTitle());
    if (item.url) {
      EXPECT_FALSE(child_node->is_folder());
      EXPECT_TRUE(child_node->is_url());
      EXPECT_EQ(child_node->url(), test_node.url());
      const base::Time expected_time =
          start_time_ + base::TimeDelta::FromMinutes(*running_count);
      EXPECT_EQ(expected_time.ToInternalValue(),
                child_node->date_added().ToInternalValue());
    } else {
      EXPECT_TRUE(child_node->is_folder());
      EXPECT_FALSE(child_node->is_url());
    }
    (*running_count)++;
  }
}

// TODO(munjal): We should implement some way of generating random data and can
// use the same seed to generate the same sequence.
void ProfileSyncServiceBookmarkTestWithData::WriteTestDataToBookmarkModel() {
  const BookmarkNode* bookmarks_bar_node = model_->bookmark_bar_node();
  int count = 0;
  PopulateFromTestData(bookmarks_bar_node,
                       kBookmarkBarChildren,
                       arraysize(kBookmarkBarChildren),
                       &count);

  ASSERT_GE(bookmarks_bar_node->child_count(), 4);
  const BookmarkNode* f1_node = bookmarks_bar_node->GetChild(1);
  PopulateFromTestData(f1_node, kF1Children, arraysize(kF1Children), &count);
  const BookmarkNode* f2_node = bookmarks_bar_node->GetChild(3);
  PopulateFromTestData(f2_node, kF2Children, arraysize(kF2Children), &count);

  const BookmarkNode* other_bookmarks_node = model_->other_node();
  PopulateFromTestData(other_bookmarks_node,
                       kOtherBookmarkChildren,
                       arraysize(kOtherBookmarkChildren),
                       &count);

  ASSERT_GE(other_bookmarks_node->child_count(), 6);
  const BookmarkNode* f3_node = other_bookmarks_node->GetChild(0);
  PopulateFromTestData(f3_node, kF3Children, arraysize(kF3Children), &count);
  const BookmarkNode* f4_node = other_bookmarks_node->GetChild(3);
  PopulateFromTestData(f4_node, kF4Children, arraysize(kF4Children), &count);
  const BookmarkNode* dup_node = other_bookmarks_node->GetChild(4);
  PopulateFromTestData(dup_node, kDup1Children, arraysize(kDup1Children),
                       &count);
  dup_node = other_bookmarks_node->GetChild(5);
  PopulateFromTestData(dup_node, kDup2Children, arraysize(kDup2Children),
                       &count);

  const BookmarkNode* mobile_bookmarks_node = model_->mobile_node();
  PopulateFromTestData(mobile_bookmarks_node,
                       kMobileBookmarkChildren,
                       arraysize(kMobileBookmarkChildren),
                       &count);

  ASSERT_GE(mobile_bookmarks_node->child_count(), 3);
  const BookmarkNode* f5_node = mobile_bookmarks_node->GetChild(0);
  PopulateFromTestData(f5_node, kF5Children, arraysize(kF5Children), &count);
  const BookmarkNode* f6_node = mobile_bookmarks_node->GetChild(1);
  PopulateFromTestData(f6_node, kF6Children, arraysize(kF6Children), &count);

  ExpectBookmarkModelMatchesTestData();
}

void ProfileSyncServiceBookmarkTestWithData::
    ExpectBookmarkModelMatchesTestData() {
  const BookmarkNode* bookmark_bar_node = model_->bookmark_bar_node();
  int count = 0;
  CompareWithTestData(bookmark_bar_node,
                      kBookmarkBarChildren,
                      arraysize(kBookmarkBarChildren),
                      &count);

  ASSERT_GE(bookmark_bar_node->child_count(), 4);
  const BookmarkNode* f1_node = bookmark_bar_node->GetChild(1);
  CompareWithTestData(f1_node, kF1Children, arraysize(kF1Children), &count);
  const BookmarkNode* f2_node = bookmark_bar_node->GetChild(3);
  CompareWithTestData(f2_node, kF2Children, arraysize(kF2Children), &count);

  const BookmarkNode* other_bookmarks_node = model_->other_node();
  CompareWithTestData(other_bookmarks_node,
                      kOtherBookmarkChildren,
                      arraysize(kOtherBookmarkChildren),
                      &count);

  ASSERT_GE(other_bookmarks_node->child_count(), 6);
  const BookmarkNode* f3_node = other_bookmarks_node->GetChild(0);
  CompareWithTestData(f3_node, kF3Children, arraysize(kF3Children), &count);
  const BookmarkNode* f4_node = other_bookmarks_node->GetChild(3);
  CompareWithTestData(f4_node, kF4Children, arraysize(kF4Children), &count);
  const BookmarkNode* dup_node = other_bookmarks_node->GetChild(4);
  CompareWithTestData(dup_node, kDup1Children, arraysize(kDup1Children),
                      &count);
  dup_node = other_bookmarks_node->GetChild(5);
  CompareWithTestData(dup_node, kDup2Children, arraysize(kDup2Children),
                      &count);

  const BookmarkNode* mobile_bookmarks_node = model_->mobile_node();
  CompareWithTestData(mobile_bookmarks_node,
                      kMobileBookmarkChildren,
                      arraysize(kMobileBookmarkChildren),
                      &count);

  ASSERT_GE(mobile_bookmarks_node->child_count(), 3);
  const BookmarkNode* f5_node = mobile_bookmarks_node->GetChild(0);
  CompareWithTestData(f5_node, kF5Children, arraysize(kF5Children), &count);
  const BookmarkNode* f6_node = mobile_bookmarks_node->GetChild(1);
  CompareWithTestData(f6_node, kF6Children, arraysize(kF6Children), &count);
}

// Tests persistence of the profile sync service by unloading the
// database and then reloading it from disk.
TEST_F(ProfileSyncServiceBookmarkTestWithData, Persistence) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, SAVE_TO_STORAGE);
  StartSync();

  WriteTestDataToBookmarkModel();

  ExpectModelMatch();

  // Force both models to discard their data and reload from disk.  This
  // simulates what would happen if the browser were to shutdown normally,
  // and then relaunch.
  StopSync();
  UnloadBookmarkModel();
  LoadBookmarkModel(LOAD_FROM_STORAGE, SAVE_TO_STORAGE);
  StartSync();

  ExpectBookmarkModelMatchesTestData();

  // With the BookmarkModel contents verified, ExpectModelMatch will
  // verify the contents of the sync model.
  ExpectModelMatch();
}

// Tests the merge case when the BookmarkModel is non-empty but the
// sync model is empty.  This corresponds to uploading browser
// bookmarks to an initially empty, new account.
TEST_F(ProfileSyncServiceBookmarkTestWithData, MergeWithEmptySyncModel) {
  // Don't start the sync service until we've populated the bookmark model.
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, SAVE_TO_STORAGE);

  WriteTestDataToBookmarkModel();

  // Restart sync.  This should trigger a merge step during
  // initialization -- we expect the browser bookmarks to be written
  // to the sync service during this call.
  StartSync();

  // Verify that the bookmark model hasn't changed, and that the sync model
  // matches it exactly.
  ExpectBookmarkModelMatchesTestData();
  ExpectModelMatch();
}

// Tests the merge case when the BookmarkModel is empty but the sync model is
// non-empty.  This corresponds (somewhat) to a clean install of the browser,
// with no bookmarks, connecting to a sync account that has some bookmarks.
TEST_F(ProfileSyncServiceBookmarkTestWithData, MergeWithEmptyBookmarkModel) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();

  WriteTestDataToBookmarkModel();

  ExpectModelMatch();

  // Force the databse to unload and write itself to disk.
  StopSync();

  // Blow away the bookmark model -- it should be empty afterwards.
  UnloadBookmarkModel();
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  EXPECT_EQ(model_->bookmark_bar_node()->child_count(), 0);
  EXPECT_EQ(model_->other_node()->child_count(), 0);
  EXPECT_EQ(model_->mobile_node()->child_count(), 0);

  // Now restart the sync service.  Starting it should populate the bookmark
  // model -- test for consistency.
  StartSync();
  ExpectBookmarkModelMatchesTestData();
  ExpectModelMatch();
}

// Tests the merge cases when both the models are expected to be identical
// after the merge.
TEST_F(ProfileSyncServiceBookmarkTestWithData, MergeExpectedIdenticalModels) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, SAVE_TO_STORAGE);
  StartSync();
  WriteTestDataToBookmarkModel();
  ExpectModelMatch();
  StopSync();
  UnloadBookmarkModel();

  // At this point both the bookmark model and the server should have the
  // exact same data and it should match the test data.
  LoadBookmarkModel(LOAD_FROM_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();
  ExpectBookmarkModelMatchesTestData();
  ExpectModelMatch();
  StopSync();
  UnloadBookmarkModel();

  // Now reorder some bookmarks in the bookmark model and then merge. Make
  // sure we get the order of the server after merge.
  LoadBookmarkModel(LOAD_FROM_STORAGE, DONT_SAVE_TO_STORAGE);
  ExpectBookmarkModelMatchesTestData();
  const BookmarkNode* bookmark_bar = model_->bookmark_bar_node();
  ASSERT_TRUE(bookmark_bar);
  ASSERT_GT(bookmark_bar->child_count(), 1);
  model_->Move(bookmark_bar->GetChild(0), bookmark_bar, 1);
  StartSync();
  ExpectModelMatch();
  ExpectBookmarkModelMatchesTestData();
}

// Tests the merge cases when both the models are expected to be identical
// after the merge.
TEST_F(ProfileSyncServiceBookmarkTestWithData, MergeModelsWithSomeExtras) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  WriteTestDataToBookmarkModel();
  ExpectBookmarkModelMatchesTestData();

  // Remove some nodes and reorder some nodes.
  const BookmarkNode* bookmark_bar_node = model_->bookmark_bar_node();
  int remove_index = 2;
  ASSERT_GT(bookmark_bar_node->child_count(), remove_index);
  const BookmarkNode* child_node = bookmark_bar_node->GetChild(remove_index);
  ASSERT_TRUE(child_node);
  ASSERT_TRUE(child_node->is_url());
  model_->Remove(bookmark_bar_node, remove_index);
  ASSERT_GT(bookmark_bar_node->child_count(), remove_index);
  child_node = bookmark_bar_node->GetChild(remove_index);
  ASSERT_TRUE(child_node);
  ASSERT_TRUE(child_node->is_folder());
  model_->Remove(bookmark_bar_node, remove_index);

  const BookmarkNode* other_node = model_->other_node();
  ASSERT_GE(other_node->child_count(), 1);
  const BookmarkNode* f3_node = other_node->GetChild(0);
  ASSERT_TRUE(f3_node);
  ASSERT_TRUE(f3_node->is_folder());
  remove_index = 2;
  ASSERT_GT(f3_node->child_count(), remove_index);
  model_->Remove(f3_node, remove_index);
  ASSERT_GT(f3_node->child_count(), remove_index);
  model_->Remove(f3_node, remove_index);

  StartSync();
  ExpectModelMatch();
  StopSync();

  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  WriteTestDataToBookmarkModel();
  ExpectBookmarkModelMatchesTestData();

  // Remove some nodes and reorder some nodes.
  bookmark_bar_node = model_->bookmark_bar_node();
  remove_index = 0;
  ASSERT_GT(bookmark_bar_node->child_count(), remove_index);
  child_node = bookmark_bar_node->GetChild(remove_index);
  ASSERT_TRUE(child_node);
  ASSERT_TRUE(child_node->is_url());
  model_->Remove(bookmark_bar_node, remove_index);
  ASSERT_GT(bookmark_bar_node->child_count(), remove_index);
  child_node = bookmark_bar_node->GetChild(remove_index);
  ASSERT_TRUE(child_node);
  ASSERT_TRUE(child_node->is_folder());
  model_->Remove(bookmark_bar_node, remove_index);

  ASSERT_GE(bookmark_bar_node->child_count(), 2);
  model_->Move(bookmark_bar_node->GetChild(0), bookmark_bar_node, 1);

  other_node = model_->other_node();
  ASSERT_GE(other_node->child_count(), 1);
  f3_node = other_node->GetChild(0);
  ASSERT_TRUE(f3_node);
  ASSERT_TRUE(f3_node->is_folder());
  remove_index = 0;
  ASSERT_GT(f3_node->child_count(), remove_index);
  model_->Remove(f3_node, remove_index);
  ASSERT_GT(f3_node->child_count(), remove_index);
  model_->Remove(f3_node, remove_index);

  ASSERT_GE(other_node->child_count(), 4);
  model_->Move(other_node->GetChild(0), other_node, 1);
  model_->Move(other_node->GetChild(2), other_node, 3);

  StartSync();
  ExpectModelMatch();

  // After the merge, the model should match the test data.
  ExpectBookmarkModelMatchesTestData();
}

// Tests that when persisted model associations are used, things work fine.
TEST_F(ProfileSyncServiceBookmarkTestWithData, ModelAssociationPersistence) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  WriteTestDataToBookmarkModel();
  StartSync();
  ExpectModelMatch();
  // Force sync to shut down and write itself to disk.
  StopSync();
  // Now restart sync. This time it should use the persistent
  // associations.
  StartSync();
  ExpectModelMatch();
}

// Tests that when persisted model associations are used, things work fine.
TEST_F(ProfileSyncServiceBookmarkTestWithData,
       ModelAssociationInvalidPersistence) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  WriteTestDataToBookmarkModel();
  StartSync();
  ExpectModelMatch();
  // Force sync to shut down and write itself to disk.
  StopSync();
  // Change the bookmark model before restarting sync service to simulate
  // the situation where bookmark model is different from sync model and
  // make sure model associator correctly rebuilds associations.
  const BookmarkNode* bookmark_bar_node = model_->bookmark_bar_node();
  model_->AddURL(bookmark_bar_node, 0, base::ASCIIToUTF16("xtra"),
                 GURL("http://www.xtra.com"));
  // Now restart sync. This time it will try to use the persistent
  // associations and realize that they are invalid and hence will rebuild
  // associations.
  StartSync();
  ExpectModelMatch();
}

TEST_F(ProfileSyncServiceBookmarkTestWithData, SortChildren) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();

  // Write test data to bookmark model and verify that the models match.
  WriteTestDataToBookmarkModel();
  const BookmarkNode* folder_added = model_->other_node()->GetChild(0);
  ASSERT_TRUE(folder_added);
  ASSERT_TRUE(folder_added->is_folder());

  ExpectModelMatch();

  // Sort the other-bookmarks children and expect that the models match.
  model_->SortChildren(folder_added);
  ExpectModelMatch();
}

// See what happens if we enable sync but then delete the "Sync Data"
// folder.
TEST_F(ProfileSyncServiceBookmarkTestWithData,
       RecoverAfterDeletingSyncDataDirectory) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, SAVE_TO_STORAGE);
  StartSync();

  WriteTestDataToBookmarkModel();

  StopSync();

  // Nuke the sync DB and reload.
  TearDown();
  SetUp();

  // First attempt fails due to a persistence error.
  EXPECT_TRUE(CreatePermanentBookmarkNodes());
  EXPECT_FALSE(AssociateModels());

  // Second attempt succeeds due to the previous error resetting the native
  // transaction version.
  model_associator_.reset();
  EXPECT_TRUE(CreatePermanentBookmarkNodes());
  EXPECT_TRUE(AssociateModels());

  // Make sure we're back in sync.  In real life, the user would need
  // to reauthenticate before this happens, but in the test, authentication
  // is sidestepped.
  ExpectBookmarkModelMatchesTestData();
  ExpectModelMatch();
}

// Verify that the bookmark model is updated about whether the
// associator is currently running.
TEST_F(ProfileSyncServiceBookmarkTest, AssociationState) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);

  ExtensiveChangesBookmarkModelObserver observer;
  model_->AddObserver(&observer);

  StartSync();

  EXPECT_EQ(1, observer.get_started());
  EXPECT_EQ(0, observer.get_completed_count_at_started());
  EXPECT_EQ(1, observer.get_completed());

  model_->RemoveObserver(&observer);
}

// Verify that the creation_time_us changes are applied in the local model at
// association time and update time.
TEST_F(ProfileSyncServiceBookmarkTestWithData, UpdateDateAdded) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  WriteTestDataToBookmarkModel();

  // Start and stop sync in order to create bookmark nodes in the sync db.
  StartSync();
  StopSync();

  // Modify the date_added field of a bookmark so it doesn't match with
  // the sync data.
  const BookmarkNode* bookmark_bar_node = model_->bookmark_bar_node();
  int remove_index = 2;
  ASSERT_GT(bookmark_bar_node->child_count(), remove_index);
  const BookmarkNode* child_node = bookmark_bar_node->GetChild(remove_index);
  ASSERT_TRUE(child_node);
  EXPECT_TRUE(child_node->is_url());
  model_->SetDateAdded(child_node, base::Time::FromInternalValue(10));

  StartSync();

  // Everything should be back in sync after model association.
  ExpectBookmarkModelMatchesTestData();
  ExpectModelMatch();

  // Now trigger a change while syncing. We add a new bookmark, sync it, then
  // updates it's creation time.
  syncer::WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
  FakeServerChange adds(&trans);
  const std::string kTitle = "Some site";
  const std::string kUrl = "http://www.whatwhat.yeah/";
  const int kCreationTime = 30;
  int64 id = adds.AddURL(kTitle, kUrl,
                         bookmark_bar_id(), 0);
  adds.ApplyPendingChanges(change_processor_.get());
  FakeServerChange updates(&trans);
  updates.ModifyCreationTime(id, kCreationTime);
  updates.ApplyPendingChanges(change_processor_.get());

  const BookmarkNode* node = model_->bookmark_bar_node()->GetChild(0);
  ASSERT_TRUE(node);
  EXPECT_TRUE(node->is_url());
  EXPECT_EQ(base::UTF8ToUTF16(kTitle), node->GetTitle());
  EXPECT_EQ(kUrl, node->url().possibly_invalid_spec());
  EXPECT_EQ(node->date_added(), base::Time::FromInternalValue(30));
}

// Tests that changes to the sync nodes meta info gets reflected in the local
// bookmark model.
TEST_F(ProfileSyncServiceBookmarkTestWithData, UpdateMetaInfoFromSync) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  WriteTestDataToBookmarkModel();
  StartSync();

  // Create bookmark nodes containing meta info.
  syncer::WriteTransaction trans(FROM_HERE, test_user_share_.user_share());
  FakeServerChange adds(&trans);
  BookmarkNode::MetaInfoMap folder_meta_info;
  folder_meta_info["folder"] = "foldervalue";
  int64 folder_id = adds.AddFolderWithMetaInfo(
      "folder title", &folder_meta_info, bookmark_bar_id(), 0);
  BookmarkNode::MetaInfoMap node_meta_info;
  node_meta_info["node"] = "nodevalue";
  node_meta_info["other"] = "othervalue";
  int64 id = adds.AddURLWithMetaInfo("node title", "http://www.foo.com",
                                     &node_meta_info, folder_id, 0);
  adds.ApplyPendingChanges(change_processor_.get());

  // Verify that the nodes are created with the correct meta info.
  ASSERT_LT(0, model_->bookmark_bar_node()->child_count());
  const BookmarkNode* folder_node = model_->bookmark_bar_node()->GetChild(0);
  ASSERT_TRUE(folder_node->GetMetaInfoMap());
  EXPECT_EQ(folder_meta_info, *folder_node->GetMetaInfoMap());
  ASSERT_LT(0, folder_node->child_count());
  const BookmarkNode* node = folder_node->GetChild(0);
  ASSERT_TRUE(node->GetMetaInfoMap());
  EXPECT_EQ(node_meta_info, *node->GetMetaInfoMap());

  // Update meta info on nodes on server
  FakeServerChange updates(&trans);
  folder_meta_info.erase("folder");
  updates.ModifyMetaInfo(folder_id, folder_meta_info);
  node_meta_info["node"] = "changednodevalue";
  node_meta_info.erase("other");
  node_meta_info["newkey"] = "newkeyvalue";
  updates.ModifyMetaInfo(id, node_meta_info);
  updates.ApplyPendingChanges(change_processor_.get());

  // Confirm that the updated values are reflected in the bookmark nodes.
  EXPECT_FALSE(folder_node->GetMetaInfoMap());
  ASSERT_TRUE(node->GetMetaInfoMap());
  EXPECT_EQ(node_meta_info, *node->GetMetaInfoMap());
}

// Tests that changes to the local bookmark nodes meta info gets reflected in
// the sync nodes.
TEST_F(ProfileSyncServiceBookmarkTestWithData, UpdateMetaInfoFromModel) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  WriteTestDataToBookmarkModel();
  StartSync();
  ExpectBookmarkModelMatchesTestData();

  const BookmarkNode* folder_node =
      model_->AddFolder(model_->bookmark_bar_node(), 0,
                        base::ASCIIToUTF16("folder title"));
  const BookmarkNode* node = model_->AddURL(folder_node, 0,
                                            base::ASCIIToUTF16("node title"),
                                            GURL("http://www.foo.com"));
  ExpectModelMatch();

  // Add some meta info and verify sync model matches the changes.
  model_->SetNodeMetaInfo(folder_node, "folder", "foldervalue");
  model_->SetNodeMetaInfo(node, "node", "nodevalue");
  model_->SetNodeMetaInfo(node, "other", "othervalue");
  ExpectModelMatch();

  // Change/delete existing meta info and verify.
  model_->DeleteNodeMetaInfo(folder_node, "folder");
  model_->SetNodeMetaInfo(node, "node", "changednodevalue");
  model_->DeleteNodeMetaInfo(node, "other");
  model_->SetNodeMetaInfo(node, "newkey", "newkeyvalue");
  ExpectModelMatch();
}

void ProfileSyncServiceBookmarkTestWithData::GetTransactionVersions(
    const BookmarkNode* root,
    BookmarkNodeVersionMap* node_versions) {
  node_versions->clear();
  std::queue<const BookmarkNode*> nodes;
  nodes.push(root);
  while (!nodes.empty()) {
    const BookmarkNode* n = nodes.front();
    nodes.pop();

    int64 version = n->sync_transaction_version();
    EXPECT_NE(BookmarkNode::kInvalidSyncTransactionVersion, version);

    (*node_versions)[n->id()] = version;
    for (int i = 0; i < n->child_count(); ++i) {
      if (!CanSyncNode(n->GetChild(i)))
        continue;
      nodes.push(n->GetChild(i));
    }
  }
}

void ProfileSyncServiceBookmarkTestWithData::ExpectTransactionVersionMatch(
    const BookmarkNode* node,
    const BookmarkNodeVersionMap& version_expected) {
  syncer::ReadTransaction trans(FROM_HERE, test_user_share_.user_share());

  BookmarkNodeVersionMap bnodes_versions;
  GetTransactionVersions(node, &bnodes_versions);
  for (BookmarkNodeVersionMap::const_iterator it = bnodes_versions.begin();
       it != bnodes_versions.end(); ++it) {
    syncer::ReadNode sync_node(&trans);
    ASSERT_TRUE(model_associator_->InitSyncNodeFromChromeId(it->first,
                                                            &sync_node));
    EXPECT_EQ(sync_node.GetEntry()->GetTransactionVersion(), it->second);
    BookmarkNodeVersionMap::const_iterator expected_ver_it =
        version_expected.find(it->first);
    if (expected_ver_it != version_expected.end())
      EXPECT_EQ(expected_ver_it->second, it->second);
  }
}

// Test transaction versions of model and nodes are incremented after changes
// are applied.
TEST_F(ProfileSyncServiceBookmarkTestWithData, UpdateTransactionVersion) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();
  WriteTestDataToBookmarkModel();
  base::MessageLoop::current()->RunUntilIdle();

  BookmarkNodeVersionMap initial_versions;

  // Verify transaction versions in sync model and bookmark model (saved as
  // transaction version of root node) are equal after
  // WriteTestDataToBookmarkModel() created bookmarks.
  {
    syncer::ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    EXPECT_GT(trans.GetModelVersion(syncer::BOOKMARKS), 0);
    GetTransactionVersions(model_->root_node(), &initial_versions);
    EXPECT_EQ(trans.GetModelVersion(syncer::BOOKMARKS),
              initial_versions[model_->root_node()->id()]);
  }
  ExpectTransactionVersionMatch(model_->bookmark_bar_node(),
                                BookmarkNodeVersionMap());
  ExpectTransactionVersionMatch(model_->other_node(),
                                BookmarkNodeVersionMap());
  ExpectTransactionVersionMatch(model_->mobile_node(),
                                BookmarkNodeVersionMap());

  // Verify model version is incremented and bookmark node versions remain
  // the same.
  const BookmarkNode* bookmark_bar = model_->bookmark_bar_node();
  model_->Remove(bookmark_bar, 0);
  base::MessageLoop::current()->RunUntilIdle();
  BookmarkNodeVersionMap new_versions;
  GetTransactionVersions(model_->root_node(), &new_versions);
  EXPECT_EQ(initial_versions[model_->root_node()->id()] + 1,
            new_versions[model_->root_node()->id()]);
  ExpectTransactionVersionMatch(model_->bookmark_bar_node(), initial_versions);
  ExpectTransactionVersionMatch(model_->other_node(), initial_versions);
  ExpectTransactionVersionMatch(model_->mobile_node(), initial_versions);

  // Verify model version and version of changed bookmark are incremented and
  // versions of others remain same.
  const BookmarkNode* changed_bookmark =
      model_->bookmark_bar_node()->GetChild(0);
  model_->SetTitle(changed_bookmark, base::ASCIIToUTF16("test"));
  base::MessageLoop::current()->RunUntilIdle();
  GetTransactionVersions(model_->root_node(), &new_versions);
  EXPECT_EQ(initial_versions[model_->root_node()->id()] + 2,
            new_versions[model_->root_node()->id()]);
  EXPECT_LT(initial_versions[changed_bookmark->id()],
            new_versions[changed_bookmark->id()]);
  initial_versions.erase(changed_bookmark->id());
  ExpectTransactionVersionMatch(model_->bookmark_bar_node(), initial_versions);
  ExpectTransactionVersionMatch(model_->other_node(), initial_versions);
  ExpectTransactionVersionMatch(model_->mobile_node(), initial_versions);
}

// Test that sync persistence errors are detected and trigger a failed
// association.
TEST_F(ProfileSyncServiceBookmarkTestWithData, PersistenceError) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();
  WriteTestDataToBookmarkModel();
  base::MessageLoop::current()->RunUntilIdle();

  BookmarkNodeVersionMap initial_versions;

  // Verify transaction versions in sync model and bookmark model (saved as
  // transaction version of root node) are equal after
  // WriteTestDataToBookmarkModel() created bookmarks.
  {
    syncer::ReadTransaction trans(FROM_HERE, test_user_share_.user_share());
    EXPECT_GT(trans.GetModelVersion(syncer::BOOKMARKS), 0);
    GetTransactionVersions(model_->root_node(), &initial_versions);
    EXPECT_EQ(trans.GetModelVersion(syncer::BOOKMARKS),
              initial_versions[model_->root_node()->id()]);
  }
  ExpectTransactionVersionMatch(model_->bookmark_bar_node(),
                                BookmarkNodeVersionMap());
  ExpectTransactionVersionMatch(model_->other_node(),
                                BookmarkNodeVersionMap());
  ExpectTransactionVersionMatch(model_->mobile_node(),
                                BookmarkNodeVersionMap());

  // Now shut down sync and artificially increment the native model's version.
  StopSync();
  int64 root_version = initial_versions[model_->root_node()->id()];
  model_->SetNodeSyncTransactionVersion(model_->root_node(), root_version + 1);

  // Upon association, bookmarks should fail to associate.
  EXPECT_FALSE(AssociateModels());
}

// It's possible for update/add calls from the bookmark model to be out of
// order, or asynchronous. Handle that without triggering an error.
TEST_F(ProfileSyncServiceBookmarkTest, UpdateThenAdd) {
  LoadBookmarkModel(DELETE_EXISTING_STORAGE, DONT_SAVE_TO_STORAGE);
  StartSync();

  EXPECT_TRUE(other_bookmarks_id());
  EXPECT_TRUE(bookmark_bar_id());
  EXPECT_TRUE(mobile_bookmarks_id());

  ExpectModelMatch();

  // Now destroy the change processor then add a bookmark, to simulate
  // missing the Update call.
  change_processor_.reset();
  const BookmarkNode* node = model_->AddURL(model_->bookmark_bar_node(),
                                            0,
                                            base::ASCIIToUTF16("title"),
                                            GURL("http://www.url.com"));

  // Recreate the change processor then update that bookmark. Sync should
  // receive the update call and gracefully treat that as if it were an add.
  change_processor_.reset(new BookmarkChangeProcessor(
      &profile_, model_associator_.get(), &mock_error_handler_));
  change_processor_->Start(test_user_share_.user_share());
  model_->SetTitle(node, base::ASCIIToUTF16("title2"));
  ExpectModelMatch();

  // Then simulate the add call arriving late.
  change_processor_->BookmarkNodeAdded(model_, model_->bookmark_bar_node(), 0);
  ExpectModelMatch();
}

}  // namespace

}  // namespace browser_sync
