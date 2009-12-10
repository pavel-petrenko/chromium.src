description("Tests calling WebGL APIs without providing the necessary objects");

var context = create3DDebugContext();
var program = loadStandardProgram(context);
var shader = loadStandardVertexShader(context);

shouldThrow("context.compileShader()");
shouldThrow("context.linkProgram()");
shouldThrow("context.attachShader()");
shouldThrow("context.attachShader(program, undefined)");
shouldThrow("context.attachShader(undefined, shader)");
shouldThrow("context.detachShader(program, undefined)");
shouldThrow("context.detachShader(undefined, shader)");
shouldThrow("context.shaderSource()");
shouldThrow("context.shaderSource(undefined, 'foo')");
shouldThrow("context.bindAttribLocation(undefined, 0, 'foo')");
shouldBeUndefined("context.bindBuffer(context.ARRAY_BUFFER, 0)");
shouldBeUndefined("context.bindFramebuffer(context.FRAMEBUFFER, 0)");
shouldBeUndefined("context.bindRenderbuffer(context.RENDERBUFFER, 0)");
shouldBeUndefined("context.bindTexture(context.TEXTURE_2D, 0)");
// FIXME: the following two tests should state shouldThrow, not shouldBeUndefined
// https://bugs.webkit.org/show_bug.cgi?id=32391
// shouldBeUndefined("context.framebufferRenderbuffer(context.FRAMEBUFFER, context.DEPTH_ATTACHMENT, context.RENDERBUFFER, 0)");
// shouldBeUndefined("context.framebufferTexture2D(context.FRAMEBUFFER, context.COLOR_ATTACHMENT0, context.TEXTURE_2D, 0, 0)");
shouldThrow("context.getProgramParameter(undefined, 0)");
shouldThrow("context.getProgramInfoLog(undefined, 0)");
shouldThrow("context.getShaderParameter(undefined, 0)");
shouldThrow("context.getShaderInfoLog(undefined, 0)");
shouldThrow("context.getShaderSource(undefined)");
shouldThrow("context.getUniform(undefined, 0)");
shouldThrow("context.getUniformLocation(undefined, 'foo')");

successfullyParsed = true;
