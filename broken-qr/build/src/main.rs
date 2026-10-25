use qrcode_generator::{
    qr::{Encoder, ErrorCorrection, Mask, Version},
    Renderer,
    Segment,
};

fn main() {
    const FLAG: &[u8] = b"ECSC{sn3ak_5egm3nt}";
    const KEY: &[u8] = &[
        0x68, 0xb2, 0xbb, 0xfd, 0x07, 0x5a, 0x4b, 0xdf, 0xda, 0xa4,
        0xac, 0x39, 0x70, 0x18, 0xfe, 0xf6, 0x75, 0x67, 0x97,
    ];
    assert_eq!(FLAG.len(), KEY.len());

    let enc_flag = FLAG.iter().zip(KEY).map(|(a, b)| a ^ b).collect::<Vec<_>>();
    let segments = [
        Segment::bytes(&KEY[13..]),
        Segment::bytes(enc_flag),
        Segment::bytes(b"S1^(S3+S0)"),
        Segment::bytes(&KEY[..13]),
    ];

    let symbol = Encoder::new(ErrorCorrection::Low)
        .version_range(Version::new(3).unwrap()..=Version::new(3).unwrap())
        .boost_error_correction(false)
        .mask(Mask::new(1).unwrap())
        .encode_segments(&segments)
        .unwrap();

    Renderer::new(&symbol, 512).save_png("output.png").unwrap();
}
