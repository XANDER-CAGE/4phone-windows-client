#pragma once

/*
 * 4phone audio-only PJSIP configuration.
 *
 * TLS uses Windows Schannel so certificate validation relies on the
 * operating system trust store and does not require bundled OpenSSL files.
 */

#define PJ_HAS_SSL_SOCK 1
#define PJ_SSL_SOCK_IMP PJ_SSL_SOCK_IMP_SCHANNEL

#define PJMEDIA_HAS_VIDEO 0
#define PJMEDIA_HAS_OPUS_CODEC 1

#define PJMEDIA_HAS_OPENCORE_AMRNB_CODEC 0
#define PJMEDIA_HAS_OPENCORE_AMRWB_CODEC 0
#define PJMEDIA_HAS_SILK_CODEC 0
#define PJMEDIA_HAS_BCG729 0
#define PJMEDIA_HAS_LYRA_CODEC 0
