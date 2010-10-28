/*
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.ResourceManager = function()
{
    this._registerNotifyHandlers(
        "identifierForInitialRequest",
        "willSendRequest",
        "markResourceAsCached",
        "didReceiveResponse",
        "didReceiveContentLength",
        "didFinishLoading",
        "didFailLoading",
        "didLoadResourceFromMemoryCache",
        "setOverrideContent",
        "didCommitLoadForFrame",
        "frameDetachedFromParent",
        "didCreateWebSocket",
        "willSendWebSocketHandshakeRequest",
        "didReceiveWebSocketHandshakeResponse",
        "didCloseWebSocket");

    this._resourcesById = {};
    this._resourcesByURL = {};
    this._resourceTreeModel = new WebInspector.ResourceTreeModel();
    InspectorBackend.cachedResources(this._processCachedResources.bind(this));
}

WebInspector.ResourceManager.prototype = {
    _registerNotifyHandlers: function()
    {
        for (var i = 0; i < arguments.length; ++i)
            WebInspector[arguments[i]] = this[arguments[i]].bind(this);
    },

    identifierForInitialRequest: function(identifier, url, loader)
    {
        var resource = this._createResource(identifier, url, loader);
        if (loader.url === url) {
            resource.isMainResource = true;
            WebInspector.mainResource = resource;
        }

        // It is important to bind resource url early (before scripts compile).
        this._bindResourceURL(resource);

        WebInspector.panels.network.addResource(resource);
        WebInspector.panels.audits.resourceStarted(resource);
    },

    _createResource: function(identifier, url, loader)
    {
        var resource = new WebInspector.Resource(identifier, url);
        resource.loader = loader;
        resource.documentURL = loader.url;

        this._resourcesById[identifier] = resource;
        return resource;
    },

    willSendRequest: function(identifier, time, request, redirectResponse)
    {
        var resource = this._resourcesById[identifier];
        if (!resource)
            return;

        // Redirect may have empty URL and we'd like to not crash with invalid HashMap entry.
        // See http/tests/misc/will-send-request-returns-null-on-redirect.html
        var isRedirect = !redirectResponse.isNull && request.url.length;
        if (isRedirect) {
            resource.endTime = time;
            this.didReceiveResponse(identifier, time, "Other", redirectResponse);
            resource = this._appendRedirect(resource.identifier, request.url);
        }

        this._updateResourceWithRequest(resource, request);
        resource.startTime = time;

        if (isRedirect) {
            WebInspector.panels.network.addResource(resource);
            WebInspector.panels.audits.resourceStarted(resource);
        } else 
            WebInspector.panels.network.refreshResource(resource);
    },

    _updateResourceWithRequest: function(resource, request)
    {
        resource.requestMethod = request.httpMethod;
        resource.requestHeaders = request.httpHeaderFields;
        resource.requestFormData = request.requestFormData;
    },

    _appendRedirect: function(identifier, redirectURL)
    {
        var originalResource = this._resourcesById[identifier];
        originalResource.identifier = null;

        var newResource = this._createResource(identifier, redirectURL, originalResource.loader);
        newResource.redirects = originalResource.redirects || [];
        delete originalResource.redirects;
        newResource.redirects.push(originalResource);
        return newResource;
    },

    markResourceAsCached: function(identifier)
    {
        var resource = this._resourcesById[identifier];
        if (!resource)
            return;

        resource.cached = true;
        WebInspector.panels.network.refreshResource(resource);
    },

    didReceiveResponse: function(identifier, time, resourceType, response)
    {
        var resource = this._resourcesById[identifier];
        if (!resource)
            return;
        this._updateResourceWithResponse(resource, response);
        resource.type = WebInspector.Resource.Type[resourceType];
        resource.responseReceivedTime = time;

        WebInspector.panels.network.refreshResource(resource);
        this._resourceTreeModel.addResourceToFrame(resource.loader.frameId, resource);
    },

    _updateResourceWithResponse: function(resource, response)
    {
        resource.mimeType = response.mimeType;
        resource.expectedContentLength = response.expectedContentLength;
        resource.textEncodingName = response.textEncodingName;
        resource.suggestedFilename = response.suggestedFilename;
        resource.statusCode = response.httpStatusCode;
        resource.statusText = response.httpStatusText;

        resource.responseHeaders = response.httpHeaderFields;
        resource.connectionReused = response.connectionReused;
        resource.connectionID = response.connectionID;

        if (response.wasCached)
            resource.cached = true;
        else
            resource.timing = response.timing;

        if (response.rawHeaders) {
            resource.requestHeaders = response.rawHeaders.requestHeaders;
            resource.responseHeaders = response.rawHeaders.responseHeaders;
        }
    },

    didReceiveContentLength: function(identifier, time, lengthReceived)
    {
        var resource = this._resourcesById[identifier];
        if (!resource)
            return;

        resource.resourceSize += lengthReceived;
        resource.endTime = time;

        WebInspector.panels.network.refreshResource(resource);
    },

    didFinishLoading: function(identifier, finishTime)
    {
        var resource = this._resourcesById[identifier];
        if (!resource)
            return;

        resource.finished = true;
        resource.endTime = finishTime;

        WebInspector.panels.network.refreshResource(resource);
        WebInspector.panels.audits.resourceFinished(resource);
        WebInspector.extensionServer.notifyResourceFinished(resource);
        delete this._resourcesById[identifier];
    },

    didFailLoading: function(identifier, time, localizedDescription)
    {
        var resource = this._resourcesById[identifier];
        if (!resource)
            return;

        resource.failed = true;
        resource.finished = true;
        resource.endTime = time;

        WebInspector.panels.network.refreshResource(resource);
        WebInspector.panels.audits.resourceFinished(resource);
        WebInspector.extensionServer.notifyResourceFinished(resource);
        delete this._resourcesById[identifier];
    },

    didLoadResourceFromMemoryCache: function(time, cachedResource)
    {
        var resource = this._createResource(null, cachedResource.url, cachedResource.loader);
        this._updateResourceWithCachedResource(resource, cachedResource);
        resource.cached = true;
        resource.startTime = resource.responseReceivedTime = resource.endTime = time;

        WebInspector.panels.network.addResource(resource);
        WebInspector.panels.audits.resourceStarted(resource);
        WebInspector.panels.audits.resourceFinished(resource);
        this._resourceTreeModel.addResourceToFrame(resource.loader.frameId, resource);
    },

    _updateResourceWithCachedResource: function(resource, cachedResource)
    {
        resource.type = WebInspector.Resource.Type[cachedResource.type];
        resource.resourceSize = cachedResource.encodedSize;
        this._updateResourceWithResponse(resource, cachedResource.response);
    },

    setOverrideContent: function(identifier, sourceString, type)
    {
        var resource = WebInspector.panels.network.resources[identifier];
        if (!resource)
            return;

        resource.type = WebInspector.Resource.Type[type];
        resource.content = sourceString;
        WebInspector.panels.network.refreshResource(resource);
    },

    didCommitLoadForFrame: function(parentFrameId, loader)
    {
        this._resourceTreeModel.didCommitLoadForFrame(parentFrameId, loader);
    },

    frameDetachedFromParent: function(frameId)
    {
        this._resourceTreeModel.frameDetachedFromParent(frameId);
    },

    didCreateWebSocket: function(identifier, requestURL)
    {
        this.identifierForInitialRequest(identifier, requestURL);
        var resource = this._resourcesById[identifier];
        resource.type = WebInspector.Resource.Type.WebSocket;

        WebInspector.panels.network.addResource(resource);
    },

    willSendWebSocketHandshakeRequest: function(identifier, time, request)
    {
        var resource = this._resourcesById[identifier];
        if (!resource)
            return;

        resource.requestMethod = "GET";
        resource.requestHeaders = request.webSocketHeaderFields;
        resource.webSocketRequestKey3 = request.webSocketRequestKey3;
        resource.startTime = time;

        WebInspector.panels.network.refreshResource(resource);
    },

    didReceiveWebSocketHandshakeResponse: function(identifier, time, response)
    {
        var resource = this._resourcesById[identifier];
        if (!resource)
            return;

        resource.statusCode = response.statusCode;
        resource.statusText = response.statusText;
        resource.responseHeaders = response.webSocketHeaderFields;
        resource.webSocketChallengeResponse = response.webSocketChallengeResponse;
        resource.responseReceivedTime = time;

        WebInspector.panels.network.refreshResource(resource);
    },

    didCloseWebSocket: function(identifier, time)
    {
        var resource = this._resourcesById[identifier];
        if (!resource)
            return;
        resource.endTime = time;

        WebInspector.panels.network.refreshResource(resource);
    },

    _processCachedResources: function(mainFramePayload)
    {
        var mainResource = this._addFramesRecursively(null, mainFramePayload);
        WebInspector.mainResource = mainResource;
        mainResource.isMainResource = true;
    },

    _addFramesRecursively: function(parentFrameId, framePayload)
    {
        var frameResource = this._createResource(null, framePayload.resource.url, framePayload.resource.loader);
        this._updateResourceWithRequest(frameResource, framePayload.resource.request);
        this._updateResourceWithResponse(frameResource, framePayload.resource.response);
        frameResource.type = WebInspector.Resource.Type["Document"];
        frameResource.finished = true;
        this._bindResourceURL(frameResource);

        this._resourceTreeModel.addOrUpdateFrame(parentFrameId, framePayload.id, frameResource.displayName);
        this._resourceTreeModel.addResourceToFrame(framePayload.id, frameResource);

        for (var i = 0; framePayload.children && i < framePayload.children.length; ++i)
            this._addFramesRecursively(framePayload.id, framePayload.children[i]);

        if (!framePayload.subresources)
            return;

        for (var i = 0; i < framePayload.subresources.length; ++i) {
            var cachedResource = framePayload.subresources[i];
            var resource = this._createResource(null, cachedResource.url, cachedResource.loader);
            this._updateResourceWithCachedResource(resource, cachedResource);
            resource.finished = true;
            this._bindResourceURL(resource);
            this._resourceTreeModel.addResourceToFrame(framePayload.id, resource);
        }
        return frameResource;
    },

    resourceForURL: function(url)
    {
        // FIXME: receive frameId here.
        var entry = this._resourcesByURL[url];
        if (entry instanceof Array)
            return entry[0];
        return entry;
    },

    addConsoleMessage: function(msg)
    {
        var resource = this.resourceForURL(msg.url);
        if (!resource)
            return;

        switch (msg.level) {
        case WebInspector.ConsoleMessage.MessageLevel.Warning:
            resource.warnings += msg.repeatDelta;
            break;
        case WebInspector.ConsoleMessage.MessageLevel.Error:
            resource.errors += msg.repeatDelta;
            break;
        }

        var view = WebInspector.ResourceManager.resourceViewForResource(resource);
        if (view.addMessage)
            view.addMessage(msg);
    },

    clearConsoleMessages: function()
    {
        function callback(resource)
        {
            resource.clearErrorsAndWarnings();
        }
        this._resourceTreeModel.forAllResources(callback);
    },

    forAllResources: function(callback)
    {
        this._resourceTreeModel.forAllResources(callback);
    },

    _bindResourceURL: function(resource)
    {
        var resourceForURL = this._resourcesByURL[resource.url];
        if (!resourceForURL)
            this._resourcesByURL[resource.url] = resource;
        else if (resourceForURL instanceof Array)
            resourceForURL.push(resource);
        else
            this._resourcesByURL[resource.url] = [resourceForURL];
    },

    _unbindResourceURL: function(resource)
    {
        var resourceForURL = this._resourcesByURL[resource.url];
        if (!resourceForURL)
            return;

        if (resourceForURL instanceof Array) {
            resourceForURL.remove(resource, true);
            if (resourceForURL.length === 1)
                this._resourcesByURL[resource.url] = resourceForURL[0];
            return;
        }

        delete this._resourcesByURL[resource.url];
    }
}

WebInspector.ResourceManager.createResourceView = function(resource)
{
    switch (resource.category) {
    case WebInspector.resourceCategories.documents:
    case WebInspector.resourceCategories.stylesheets:
    case WebInspector.resourceCategories.scripts:
    case WebInspector.resourceCategories.xhr:
        return new WebInspector.SourceView(resource);
    case WebInspector.resourceCategories.images:
        return new WebInspector.ImageView(resource);
    case WebInspector.resourceCategories.fonts:
        return new WebInspector.FontView(resource);
    default:
        return new WebInspector.ResourceView(resource);
    }
}

WebInspector.ResourceManager.resourceViewTypeMatchesResource = function(resource, resourceView)
{
    switch (resource.category) {
    case WebInspector.resourceCategories.documents:
    case WebInspector.resourceCategories.stylesheets:
    case WebInspector.resourceCategories.scripts:
    case WebInspector.resourceCategories.xhr:
        return resourceView.__proto__ === WebInspector.SourceView.prototype;
    case WebInspector.resourceCategories.images:
        return resourceView.__proto__ === WebInspector.ImageView.prototype;
    case WebInspector.resourceCategories.fonts:
        return resourceView.__proto__ === WebInspector.FontView.prototype;
    default:
        return resourceView.__proto__ === WebInspector.ResourceView.prototype;
    }
}

WebInspector.ResourceManager.resourceViewForResource = function(resource)
{
    if (!resource)
        return null;
    if (!resource._resourcesView)
        resource._resourcesView = WebInspector.ResourceManager.createResourceView(resource);
    return resource._resourcesView;
}

WebInspector.ResourceManager.existingResourceViewForResource = function(resource)
{
    if (!resource)
        return null;
    return resource._resourcesView;
}

WebInspector.ResourceManager.getContent = function(resource, base64Encode, callback)
{
    // FIXME: eventually, cached resources will have no identifiers.
    if (resource.loader)
        InspectorBackend.resourceContent(resource.loader.frameId, resource.url, base64Encode, callback);
    else
        InspectorBackend.getResourceContent(resource.identifier, base64Encode, callback);
}

WebInspector.ResourceTreeModel = function()
{
    this._resourcesByFrameId = {};
    this._subframes = {};
}

WebInspector.ResourceTreeModel.prototype = {
    addOrUpdateFrame: function(parentFrameId, frameId, displayName)
    {
        WebInspector.panels.storage.addOrUpdateFrame(parentFrameId, frameId, displayName);
        var subframes = this._subframes[parentFrameId];
        if (!subframes) {
            subframes = {};
            this._subframes[parentFrameId || 0] = subframes;
        }
        subframes[frameId] = true;
    },

    didCommitLoadForFrame: function(parentFrameId, loader)
    {
        this._clearChildFramesAndResources(loader.frameId, loader.loaderId);

        var tmpResource = new WebInspector.Resource(null, loader.url);
        this.addOrUpdateFrame(parentFrameId, loader.frameId, tmpResource.displayName);

        var resourcesForFrame = this._resourcesByFrameId[loader.frameId];
        for (var i = 0; resourcesForFrame && i < resourcesForFrame.length; ++i) {
            WebInspector.resourceManager._bindResourceURL(resourcesForFrame[i]);
            WebInspector.panels.storage.addResourceToFrame(loader.frameId, resourcesForFrame[i]);
        }
    },

    frameDetachedFromParent: function(frameId)
    {
        this._clearChildFramesAndResources(frameId, 0);
        WebInspector.panels.storage.removeFrame(frameId);
    },

    _clearChildFramesAndResources: function(frameId, loaderId)
    {
        WebInspector.panels.storage.removeResourcesFromFrame(frameId);

        this._clearResources(frameId, loaderId);
        var subframes = this._subframes[frameId];
        if (!subframes)
            return;

        for (var childFrameId in subframes) {
            WebInspector.panels.storage.removeFrame(childFrameId);
            this._clearChildFramesAndResources(childFrameId, loaderId);
        }
        delete this._subframes[frameId];
    },

    addResourceToFrame: function(frameId, resource)
    {
        var resourcesForFrame = this._resourcesByFrameId[frameId];
        if (!resourcesForFrame) {
            resourcesForFrame = [];
            this._resourcesByFrameId[frameId] = resourcesForFrame;
        }
        resourcesForFrame.push(resource);

        WebInspector.panels.storage.addResourceToFrame(frameId, resource);
    },

    _clearResources: function(frameId, loaderToPreserveId)
    {
        var resourcesForFrame = this._resourcesByFrameId[frameId];
        if (!resourcesForFrame)
            return;

        var preservedResourcesForFrame = [];
        for (var i = 0; i < resourcesForFrame.length; ++i) {
            var resource = resourcesForFrame[i];
            if (resource.loader.loaderId === loaderToPreserveId) {
                preservedResourcesForFrame.push(resource);
                continue;
            }
            WebInspector.resourceManager._unbindResourceURL(resource);
        }

        delete this._resourcesByFrameId[frameId];
        if (preservedResourcesForFrame.length)
            this._resourcesByFrameId[frameId] = preservedResourcesForFrame;
    },

    forAllResources: function(callback)
    {
        this._callForFrameResources(0, callback);
    },

    _callForFrameResources: function(frameId, callback)
    {
        var resources = this._resourcesByFrameId[frameId];
        for (var i = 0; resources && i < resources.length; ++i) {
            if (callback(resources[i]))
                return true;
        }
        
        var frames = this._subframes[frameId];
        if (frames) {
            for (var id in frames) {
                if (this._callForFrameResources(id, callback))
                    return true;
            }
        }
        return false;
    }
}
