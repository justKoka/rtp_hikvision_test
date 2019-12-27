NAL - Network Abstraction Level
NALU - NAL unit
VCL - Video Coding Layer
FU - NALU type (Fragmented unit) -> https://tools.ietf.org/html/rfc6184#section-5.7
AP - NALU type (Aggregated packet) -> https://tools.ietf.org/html/rfc6184#section-5.8

По камерам в основном ходят FU и single NALU, AP не замечено, но реализация их обработки присутствует.
Версии h264+ и h265+ не отличаются, соответственно, от h264 и h265 на уровне NAL. Отличается только VCL
Для версии h265 не реализована пакетизация PACI -> https://tools.ietf.org/html/draft-ietf-payload-rtp-h265-15#section-4.4.4