description("Tests that when multiple requests are waiting for permission, no callbacks are invoked until permission is allowed.");

if (window.layoutTestController)
    window.layoutTestController.setMockGeolocationPosition(51.478, -0.166, 100);

function allowPermission() {
    permissionSet = true;
    if (window.layoutTestController)
        layoutTestController.setGeolocationPermission(true);
}

var watchCallbackInvoked = false;
var oneShotCallbackInvoked = false;

navigator.geolocation.watchPosition(function() {
    if (permissionSet) {
        testPassed('Success callback invoked');
        watchCallbackInvoked = true;
        maybeFinishTest();
        return;
    }
    testFailed('Success callback invoked unexpectedly');
    finishJSTest();
}, function(err) {
    testFailed('Error callback invoked unexpectedly');
    finishJSTest();
});

navigator.geolocation.getCurrentPosition(function() {
    if (permissionSet) {
        testPassed('Success callback invoked');
        oneShotCallbackInvoked = true;
        maybeFinishTest();
        return;
    }
    testFailed('Success callback invoked unexpectedly');
    finishJSTest();
}, function(err) {
    testFailed('Error callback invoked unexpectedly');
    finishJSTest();
});

window.setTimeout(allowPermission, 100);

function maybeFinishTest() {
    if (watchCallbackInvoked && oneShotCallbackInvoked)
        finishJSTest();
}

window.jsTestIsAsync = true;
window.successfullyParsed = true;
