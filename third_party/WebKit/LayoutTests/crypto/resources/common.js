// FIXME: Delete this (tests should import their own keys).
function importTestKeys()
{
    var data = asciiToUint8Array("16 bytes of key!");
    var extractable = true;
    var keyUsages = ['wrapKey', 'unwrapKey', 'encrypt', 'decrypt', 'sign', 'verify'];

    var hmacPromise = crypto.subtle.importKey('raw', data, {name: 'hmac', hash: {name: 'sha-1'}}, extractable, keyUsages);
    var aesCbcPromise = crypto.subtle.importKey('raw', data, {name: 'AES-CBC'}, extractable, keyUsages);
    var aesCbcJustDecrypt = crypto.subtle.importKey('raw', data, {name: 'AES-CBC'}, false, ['decrypt']);
    // FIXME: use AES-CTR key type once it's implemented
    var aesCtrPromise = crypto.subtle.importKey('raw', data, {name: 'AES-CBC'}, extractable, keyUsages);
    var aesGcmPromise = crypto.subtle.importKey('raw', data, {name: 'AES-GCM'}, extractable, keyUsages);
    var rsaSsaSha1PublicPromise = crypto.subtle.importKey('spki', hexStringToUint8Array(kKeyData.rsa1.spki), {name: 'RSASSA-PKCS1-v1_5', hash: {name: 'sha-1'}}, extractable, keyUsages);
    var rsaSsaSha1PrivatePromise = crypto.subtle.importKey('pkcs8', hexStringToUint8Array(kKeyData.rsa1.pkcs8), {name: 'RSASSA-PKCS1-v1_5', hash: {name: 'sha-1'}}, extractable, keyUsages);

    return Promise.all([hmacPromise, aesCbcPromise, aesCbcJustDecrypt, aesCtrPromise, aesGcmPromise, rsaSsaSha1PublicPromise, rsaSsaSha1PrivatePromise]).then(function(results) {
        return {
            hmacSha1: results[0],
            aesCbc: results[1],
            aesCbcJustDecrypt: results[2],
            aesCtr: results[3],
            aesGcm: results[4],
            rsaSsaSha1Public: results[5],
            rsaSsaSha1Private: results[6],
        };
    });
}

// Verifies that the given "bytes" holds the same value as "expectedHexString".
// "bytes" can be anything recognized by "bytesToHexString()".
function bytesShouldMatchHexString(testDescription, expectedHexString, bytes)
{
    expectedHexString = "[" + expectedHexString.toLowerCase() + "]";
    var actualHexString = "[" + bytesToHexString(bytes) + "]";

    if (actualHexString === expectedHexString) {
        debug("PASS: " + testDescription + " should be " + expectedHexString + " and was");
    } else {
        debug("FAIL: " + testDescription + " should be " + expectedHexString + " but was " + actualHexString);
    }
}

// Builds a hex string representation for an array-like input.
// "bytes" can be an Array of bytes, an ArrayBuffer, or any TypedArray.
// The output looks like this:
//    ab034c99
function bytesToHexString(bytes)
{
    if (!bytes)
        return null;

    bytes = new Uint8Array(bytes);
    var hexBytes = [];

    for (var i = 0; i < bytes.length; ++i) {
        var byteString = bytes[i].toString(16);
        if (byteString.length < 2)
            byteString = "0" + byteString;
        hexBytes.push(byteString);
    }

    return hexBytes.join("");
}

function hexStringToUint8Array(hexString)
{
    if (hexString.length % 2 != 0)
        throw "Invalid hexString";
    var arrayBuffer = new Uint8Array(hexString.length / 2);

    for (var i = 0; i < hexString.length; i += 2) {
        var byteValue = parseInt(hexString.substr(i, 2), 16);
        if (byteValue == NaN)
            throw "Invalid hexString";
        arrayBuffer[i/2] = byteValue;
    }

    return arrayBuffer;
}

function asciiToUint8Array(str)
{
    var chars = [];
    for (var i = 0; i < str.length; ++i)
        chars.push(str.charCodeAt(i));
    return new Uint8Array(chars);
}

function failAndFinishJSTest(error)
{
    if (error)
       debug(error);
    finishJSTest();
}

// =====================================================
// FIXME: Delete the addTask() functions (better to test results directly)
// =====================================================

numOutstandingTasks = 0;

function addTask(promise)
{
    numOutstandingTasks++;

    function taskFinished()
    {
        numOutstandingTasks--;
        completeTestWhenAllTasksDone();
    }

    promise.then(taskFinished, taskFinished);
}

function completeTestWhenAllTasksDone()
{
    if (numOutstandingTasks == 0) {
        finishJSTest();
    }
}

function shouldRejectPromiseWithNull(code)
{
    var promise = eval(code);

    function acceptCallback(result)
    {
        debug("FAIL: '" + code + "' accepted with " + result + " but should have been rejected");
    }

    function rejectCallback(result)
    {
        if (result == null)
            debug("PASS: '" + code + "' rejected with null");
        else
            debug("FAIL: '" + code + "' rejected with " + result + " but was expecting null");
    }

    addTask(promise.then(acceptCallback, rejectCallback));
}

function shouldAcceptPromise(code)
{
    var promise = eval(code);

    function acceptCallback(result)
    {
        debug("PASS: '" + code + "' accepted with " + result);
    }

    function rejectCallback(result)
    {
        debug("FAIL: '" + code + "' rejected with " + result);
    }

    addTask(promise.then(acceptCallback, rejectCallback));
}

// =====================================================

// Returns a Promise for the cloned key.
function cloneKey(key)
{
    // Sending an object through a MessagePort implicitly clones it.
    // Use a single MessageChannel so requests complete in FIFO order.
    var self = cloneKey;
    if (!self.channel) {
        self.channel = new MessageChannel();
        self.callbacks = [];
        self.channel.port1.addEventListener('message', function(e) {
            var callback = self.callbacks.shift();
            callback(e.data);
        }, false);
        self.channel.port1.start();
    }

    return new Promise(function(resolve, reject) {
        self.callbacks.push(resolve);
        self.channel.port2.postMessage(key);
    });
}

// Logging the serialized format ensures that if it changes it will break tests.
function logSerializedKey(o)
{
    if (internals)
        debug("Serialized key bytes: " + bytesToHexString(internals.serializeObject(o)));
}

function shouldEvaluateAs(actual, expectedValue)
{
    if (typeof expectedValue == "string")
        return shouldBeEqualToString(actual, expectedValue);
    return shouldEvaluateTo(actual, expectedValue);
}
