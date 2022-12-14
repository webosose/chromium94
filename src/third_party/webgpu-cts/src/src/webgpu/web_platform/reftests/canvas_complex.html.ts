import { unreachable } from '../../../common/util/util.js';

import { runRefTest } from './gpu_ref_test.js';

// <canvas> element from html page
declare const cvs: HTMLCanvasElement;

export function run(format: GPUTextureFormat) {
  runRefTest(async t => {
    const ctx = (cvs.getContext('webgpu') as unknown) as GPUCanvasContext;

    switch (format) {
      case 'bgra8unorm':
      case 'rgba16float':
        break;
      default:
        unreachable();
    }

    ctx.configure({
      device: t.device,
      format,
      usage: GPUTextureUsage.COPY_DST,
    });

    const rows = 2;
    const bytesPerRow = 256;
    const buffer = t.device.createBuffer({
      mappedAtCreation: true,
      size: rows * bytesPerRow,
      usage: GPUBufferUsage.COPY_SRC,
    });
    const mapping = buffer.getMappedRange();
    switch (format) {
      case 'bgra8unorm':
        {
          const data = new Uint8Array(mapping);
          data.set(new Uint8Array([0x00, 0x00, 0x7f, 0xff]), 0); // red
          data.set(new Uint8Array([0x00, 0x7f, 0x00, 0xff]), 4); // green
          data.set(new Uint8Array([0x7f, 0x00, 0x00, 0xff]), 256 + 0); // blue
          data.set(new Uint8Array([0x00, 0x7f, 0x7f, 0xff]), 256 + 4); // yellow
        }
        break;
      case 'rgba16float':
        {
          // Untested!
          const zero = 0x0000;
          const half = 0x3800;
          const one = 0x3c00;
          const data = new DataView(mapping);
          data.setUint16(0x000, half, false); // red
          data.setUint16(0x002, zero, false);
          data.setUint16(0x004, zero, false);
          data.setUint16(0x008, one, false);
          data.setUint16(0x010, zero, false); // green
          data.setUint16(0x020, half, false);
          data.setUint16(0x040, zero, false);
          data.setUint16(0x080, one, false);
          data.setUint16(0x100, zero, false); // blue
          data.setUint16(0x102, zero, false);
          data.setUint16(0x104, half, false);
          data.setUint16(0x108, one, false);
          data.setUint16(0x110, half, false); // yellow
          data.setUint16(0x120, half, false);
          data.setUint16(0x140, zero, false);
          data.setUint16(0x180, one, false);
        }
        break;
    }
    buffer.unmap();

    const texture = ctx.getCurrentTexture();

    const encoder = t.device.createCommandEncoder();
    encoder.copyBufferToTexture({ buffer, bytesPerRow }, { texture }, [2, 2, 1]);
    t.device.queue.submit([encoder.finish()]);
  });
}
