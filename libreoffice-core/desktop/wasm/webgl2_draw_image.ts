function loadShader_(
  gl: WebGL2RenderingContext,
  type:
    | WebGL2RenderingContext['VERTEX_SHADER']
    | WebGL2RenderingContext['FRAGMENT_SHADER'],
  source: string
): WebGLShader {
  const shader = gl.createShader(type)!;
  gl.shaderSource(shader, source);
  gl.compileShader(shader);
  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    console.error(
      'An error occurred compiling the shaders: ' + gl.getShaderInfoLog(shader)
    );
    gl.deleteShader(shader);
    throw new Error('Shader compile error');
  }
  return shader;
}

interface DrawImageProgram {
  program: WebGLProgram;
  // GPU input/output locations for vertex/fragment shaders
  // array buffer
  positionLocation: number;
  positionBuffer: WebGLBuffer;
  texcoordLocation: number;
  texcoordBuffer: WebGLBuffer;
  // uniform
  matrixLocation: WebGLUniformLocation;
  textureMatrixLocation: WebGLUniformLocation;
  textureLocation: WebGLUniformLocation;
}

type TexturePoolDim = 256 | 512 | 1024 | 2048;
interface TexturePoolInformation {
  gl: WebGL2RenderingContext;
  dim: TexturePoolDim;
}
export interface PooledTexture {
  tex: WebGLTexture;
  pool: TexturePoolInformation;
  release: () => void;
  valid: boolean;
}
export interface Image {
  tex: PooledTexture;
  width: number;
  height: number;
}

export interface TexturePool {
  /** acquire a texture from the pool
   * @returns if a texture is available, returns it otherwise returns `undefined`
   */
  acquire: () => PooledTexture | undefined;
}

/**
 * Prepares a WebGL2 texture pool
 * @param gl The WebGL2 context
 * @param size the number of textures allocated to the pool
 * @param dim the dimension of the texture used for height and width
 */
export function createTexturePool(
  gl: WebGL2RenderingContext,
  size: number,
  dim: TexturePoolDim
): TexturePool {
  const pool = new Array(size);
  const freeStack = new Array(size);
  let freeStackHead = 0;
  function pushFree(i: number) {
    freeStack[freeStackHead--] = i;
  }
  function popFree() {
    return freeStack[freeStackHead++];
  }
  function acquire(): PooledTexture | undefined {
    const i = popFree();
    return {
      tex: pool[i],
      pool: {
        dim,
        gl,
      },
      valid: true,
      release() {
        pushFree(i);
        this.valid = false;
      },
    };
  }

  for (let i = 0; i < size; i++) {
    const tex = gl.createTexture()!;
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texStorage2D(gl.TEXTURE_2D, 1, gl.RGBA8, dim, dim);

    pool[i] = tex;
    freeStack[i] = i;
  }

  return {
    acquire,
  };
}

export function loadImage(
  pool: TexturePool,
  data: Uint8Array,
  width: number,
  height: number,
  xOffset = 0,
  yOffset = 0
): Image {
  const pooled = pool.acquire();
  if (!pooled) {
    throw new Error('Texture pool exhausted');
  }
  const { gl } = pooled.pool;
  gl.bindTexture(gl.TEXTURE_2D, pooled.tex);
  gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, true);

  gl.texSubImage2D(
    gl.TEXTURE_2D,
    0,
    xOffset,
    yOffset,
    width,
    height,
    gl.RGBA,
    gl.UNSIGNED_BYTE,
    data
  );

  return {
    tex: pooled,
    width,
    height,
  };
}

/** simplified orthographic map, ignoring z */
function orthographic(
  left: number,
  right: number,
  bottom: number,
  top: number
): Float32Array {
  // prettier-ignore
  return new Float32Array([
    2 / (right - left), 0, 0, 0,
    0, 2 / (top - bottom), 0, 0,
    0, 0, -1, 0,
    (left + right) / (left - right), (bottom + top) / (bottom - top), 0, 1
  ]);
}

/** 2d translate from an ortho map */
function translate(m: Float32Array, tx: number, ty: number): Float32Array {
  const dst = new Float32Array(16);

  dst.set(m.subarray(0, 12));

  dst[12] = m[0] * tx + m[4] * ty + m[12];
  dst[13] = m[1] * tx + m[5] * ty + m[13];
  dst[14] = m[2] * tx + m[6] * ty + m[14];
  dst[15] = m[3] * tx + m[7] * ty + m[15];

  return dst;
}

/** 2d scale */
function scale(m: Float32Array, sx: number, sy: number): Float32Array {
  const dst = new Float32Array(16);

  for (let i = 0; i < 4; i++) {
    dst[i] = sx * m[i];
    dst[4 + i] = sy * m[4 + i];
  }

  dst.set(m.subarray(8, 16), 8); // z is unchanged

  return dst;
}

/** 2d translation matrix */
function translation(tx: number, ty: number): Float32Array {
  // prettier-ignore
  return new Float32Array([
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    tx, ty, 0, 1
  ]);
}

function flipY_(m: Float32Array): Float32Array {
  for (let i = 0; i < 4; i++) {
    m[1 + i * 4] = -m[1 + i * 4]; // Negate the second column to flip Y
  }
  return m;
}

export type DrawImageOptions = Partial<{
  srcX: number;
  srcY: number;
  srcWidth: number;
  srcHeight: number;
  dstX: number;
  dstY: number;
  dstWidth: number;
  dstHeight: number;
  /** useful when compositing to a texture which by
   * default is bottom to top instead of top to bottom */
  flipY: boolean;
}>;

function drawImage(
  gl: WebGL2RenderingContext,
  program: DrawImageProgram,
  image: Image,
  {
    dstX = 0,
    dstY = 0,
    srcX = 0,
    srcY = 0,
    srcWidth,
    srcHeight,
    dstHeight,
    dstWidth,
    flipY = false,
  }: DrawImageOptions = {}
): void {
  if (!srcWidth) {
    srcWidth = image.width;
  }
  if (!srcHeight) {
    srcHeight = image.height;
  }
  if (!dstWidth) {
    dstWidth = srcWidth;
  }
  if (!dstHeight) {
    dstHeight = srcHeight;
  }

  gl.bindTexture(gl.TEXTURE_2D, image.tex.tex);
  gl.useProgram(program.program);

  const {
    positionBuffer,
    positionLocation,
    texcoordBuffer,
    texcoordLocation,
    textureLocation,
    textureMatrixLocation,
    matrixLocation,
  } = program;

  const { width, height } = (
    gl as WebGL2RenderingContext & {
      canvas: OffscreenCanvas;
    }
  ).canvas;

  // image transform matrix
  let matrix = orthographic(0, width, height, 0); // [0,1]->[0, w/h in pixels]
  matrix = translate(matrix, dstX, dstY);
  matrix = scale(matrix, dstWidth, dstHeight);
  if (flipY) matrix = flipY_(matrix);

  const { dim } = image.tex.pool;
  // image to texture transform matrix
  let texMatrix = translation(srcX / dim, srcY / dim);
  texMatrix = scale(texMatrix, srcWidth / dim, srcHeight / dim);

  // set array buffers inputs (buffer data is set in createDrawImageProgram)
  gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
  gl.enableVertexAttribArray(positionLocation);
  gl.vertexAttribPointer(positionLocation, 2, gl.FLOAT, false, 0, 0);
  gl.bindBuffer(gl.ARRAY_BUFFER, texcoordBuffer);
  gl.enableVertexAttribArray(texcoordLocation);
  gl.vertexAttribPointer(texcoordLocation, 2, gl.FLOAT, false, 0, 0);

  // set uniform inputs
  gl.uniformMatrix4fv(matrixLocation, false, matrix);
  gl.uniformMatrix4fv(textureMatrixLocation, false, texMatrix);
  gl.uniform1i(textureLocation, 0); // texture0

  gl.drawArrays(gl.TRIANGLES, 0, 6);
}

export type drawImageFunc = (image: Image, options: DrawImageOptions) => void;

/**
 * Initializes and create a 2D image drawing function
 * @param gl The WebGL2 context used for drawing
 * @returns The drawImage function for the context
 */
export function createDrawImageProgram(
  gl: WebGL2RenderingContext
): drawImageFunc {
  // vertex shader that transforms a texture using 4x4 matrix
  const vertexShader = loadShader_(
    gl,
    gl.VERTEX_SHADER,
    `
attribute vec4 a_position;
attribute vec2 a_texcoord;

uniform mat4 u_matrix;
uniform mat4 u_textureMatrix;

varying vec2 v_texcoord;

void main() {
    gl_Position = u_matrix * a_position;
    v_texcoord = (u_textureMatrix * vec4(a_texcoord, 0, 1)).xy;
}
  `
  );

  // fragment sharder that renders a 2D texture at specified coordinates
  const fragmentShader = loadShader_(
    gl,
    gl.FRAGMENT_SHADER,
    `
precision mediump float;

varying vec2 v_texcoord;

uniform sampler2D u_texture;

void main() {
   gl_FragColor = texture2D(u_texture, v_texcoord);
}
`
  );

  const program = gl.createProgram()!;
  gl.attachShader(program, vertexShader);
  gl.attachShader(program, fragmentShader);
  gl.linkProgram(program);

  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
    console.error(
      'Unable to initialize the shader program: ' +
        gl.getProgramInfoLog(program)
    );
    throw new Error('Shader program link error');
  }

  // 2 triangles, 6 vertices forming a quad
  // prettier-ignore
  const unitQuad = [
    0, 0,
    0, 1,
    1, 0,
    1, 0,
    0, 1,
    1, 1,
  ];

  // allocate a_position
  const positionBuffer = gl.createBuffer()!;
  gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(unitQuad), gl.STATIC_DRAW);

  // allocate a_texcoord
  const texcoordBuffer = gl.createBuffer()!;
  if (!texcoordBuffer) {
    throw new Error('Unable to create texcoord buffer');
  }
  gl.bindBuffer(gl.ARRAY_BUFFER, texcoordBuffer);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(unitQuad), gl.STATIC_DRAW);

  /** fooz */
  return drawImage.bind(undefined, gl, {
    program,
    positionLocation: gl.getAttribLocation(program, 'a_position'),
    positionBuffer,
    texcoordLocation: gl.getAttribLocation(program, 'a_texcoord'),
    texcoordBuffer,
    matrixLocation: gl.getUniformLocation(program, 'u_matrix')!,
    textureMatrixLocation: gl.getUniformLocation(program, 'u_textureMatrix')!,
    textureLocation: gl.getUniformLocation(program, 'u_texture')!,
  });
}

export function clear(gl: WebGL2RenderingContext) {
  gl.clearColor(0, 0, 0, 0);
  gl.clear(gl.COLOR_BUFFER_BIT);
}
