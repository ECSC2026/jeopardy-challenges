import jsQR from "jsqr";
import { getSync } from '@andreekeberg/imagedata';
const data = getSync("../public/output.png");
const decoded = jsQR(data.data, 512, 512);
console.log(decoded);
console.log(decoded.chunks[0].bytes);
console.log(decoded.chunks[1].bytes);
console.log(decoded.chunks[2].bytes);
console.log(decoded.chunks[3].bytes);

const S0 = decoded.chunks[0].bytes;
const S1 = decoded.chunks[1].bytes;
const S2 = decoded.chunks[2].bytes;
const S3 = decoded.chunks[3].bytes;
const S3S0 = new Array(...S3, ...S0);

let output = new Array(S1.length);
for (let i = 0; i < S1.length; i++) {
    output[i] = S1[i] ^ S3S0[i];
}
console.log(Buffer.from(output).toString());
