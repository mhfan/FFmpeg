#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "libavcodec/avcodec.h"
#include "libavutil/intreadwrite.h"

#include "SsbSipH264Decode.h"
#include "SsbSipMpeg4Decode.h"
#include "SsbSipVC1Decode.h"
#include "FrameExtractor.h"
#include "MPEG4Frames.h"
#include "H263Frames.h"
#include "H264Frames.h"
#include "VC1Frames.h"
#include "SsbSipLogMsg.h"
#include "MfcDriver.h"
#include "s3c_pp.h"
#include "FileRead.h"

#define INPUT_BUFFER_SIZE (204800)

//#define OUTPUT_TYPE_FILE
#define OUT_PATH_DMA

static unsigned char delimiter_mpeg4[3] = {0x00, 0x00, 0x01};
static unsigned char delimiter_h264[4]  = {0x00, 0x00, 0x00, 0x01};

typedef void * (mfc_decode_init)();
typedef int (mfc_decode_exe)(void *openHandle, long lengthBufFill);
typedef int (mfc_decode_deinit)(void *openHandle);
typedef int (mfc_decode_set_config)(void *openHandle, H264_DEC_CONF conf_type, void *value);
typedef int (mfc_decode_get_config)(void *openHandle, H264_DEC_CONF conf_type, void *value);
typedef void *(mfc_decode_get_inbuf)(void *openHandle, long size);
typedef void *(mfc_decode_get_outbuf)(void *openHandle, long *size);

typedef int (mfc_extract_config_stream)(FRAMEX_CTX  *pFrameExCtx, void *fp, unsigned char buf[], int buf_size, H264_CONFIG_DATA *conf_data);
typedef int (mfc_next_frame)(FRAMEX_CTX  *pFrameExCtx, void *fp, unsigned char buf[], int buf_size, unsigned int *coding_type);

typedef struct MFCContext {
    void *handle;
    void *stream_buf;
    int frame_len;
    unsigned int yuv_buf[2];
    long yuv_len;
    char *in_addr;
    FRAMEX_CTX *frameex_ctx;  // frame extractor context
    //FRAMEX_STRM_PTR file_strm;
    MMAP_STRM_PTR file_strm;
    //SSBSIP_H264_STREAM_INFO stream_info; 
    SSBSIP_MPEG4_STREAM_INFO stream_info;
#ifdef OUTPUT_TYPE_FILE
    int out_fd;
#endif //OUTPUT_TYPE_FILE
    
    //delimiter
    unsigned char *delimiter_type;
    unsigned char  delimiter_size;

    //functions:
    mfc_decode_init *decode_init;
    mfc_decode_exe *decode_exe;
    mfc_decode_deinit *decode_deinit;
    mfc_decode_set_config *decode_set_config;
    mfc_decode_get_config *decode_get_config;
    mfc_decode_get_inbuf *decode_get_inbuf;
    mfc_decode_get_outbuf *decode_get_outbuf;
    mfc_extract_config_stream *extract_config_stream;
    mfc_next_frame *next_frame;

    //ioctl codes
    unsigned int MFC_DEC_GETCONF_STREAMINFO;
    unsigned int MFC_DEC_GETCONF_PHYADDR_FRAM_BUF;

    //error codes
    unsigned int SSBSIP_DEC_RET_OK;
    
    int init;
} MFCContext;

uint8_t *phy_addr;

//for bitstream filter
static AVBitStreamFilterContext *bsfc;
static uint8_t *rawbuf;
static uint32_t rawbuf_size;

static av_cold int
MFC_decode_init(AVCodecContext *avctx)
{
    AVCodec *codec = avctx->codec;
    MFCContext *mfc = avctx->priv_data;

    avctx->pix_fmt = PIX_FMT_YUV420P;

    mfc->init = 0;
    switch(codec->id){
    case CODEC_ID_H264:
        //H.264
        //retrieve SPS's profile_idc from extradata if exist
        if(avctx->extradata && avctx->extradata_size >= 6){
            const uint8_t *extradata;
            uint8_t unit_nb, unit_type, profile_idc;
            
            extradata = avctx->extradata + 5;
            unit_nb = *extradata & 0x1f;
            if(unit_nb){
                extradata += 3;
                unit_type = *extradata++ & 0x1f;
                profile_idc = AV_RB8(extradata);
                if ((unit_type == 7) && (profile_idc != 66)){
                    av_log(avctx, AV_LOG_ERROR, "%d, Only support H.264 Baseline Profile!\n", profile_idc);
                    return -1;
                }
            }
        }

        mfc->frame_len = 0;
        mfc->delimiter_type = delimiter_h264;
        mfc->delimiter_size = sizeof(delimiter_h264);
        mfc->decode_init = SsbSipH264DecodeInit;
        mfc->decode_exe = SsbSipH264DecodeExe;
        mfc->decode_deinit = SsbSipH264DecodeDeInit;
        mfc->decode_get_config  = SsbSipH264DecodeGetConfig;
        mfc->decode_get_inbuf = SsbSipH264DecodeGetInBuf;
        mfc->decode_get_outbuf = SsbSipH264DecodeGetOutBuf;
        mfc->extract_config_stream = ExtractConfigStreamH264;
        mfc->next_frame = NextFrameH264;
        mfc->MFC_DEC_GETCONF_STREAMINFO = H264_DEC_GETCONF_STREAMINFO;
        mfc->MFC_DEC_GETCONF_PHYADDR_FRAM_BUF = H264_DEC_GETCONF_PHYADDR_FRAM_BUF;
        mfc->SSBSIP_DEC_RET_OK = SSBSIP_H264_DEC_RET_OK;
        break;
    case CODEC_ID_MPEG4:
        //MPEG4 
        mfc->frame_len = 0;
        mfc->delimiter_type = delimiter_mpeg4;
        mfc->delimiter_size = sizeof(delimiter_mpeg4);
        mfc->decode_init = SsbSipMPEG4DecodeInit;
        mfc->decode_exe = SsbSipMPEG4DecodeExe;
        mfc->decode_deinit = SsbSipMPEG4DecodeDeInit;
        mfc->decode_get_config  = SsbSipMPEG4DecodeGetConfig;
        mfc->decode_get_inbuf = SsbSipMPEG4DecodeGetInBuf;
        mfc->decode_get_outbuf = SsbSipMPEG4DecodeGetOutBuf;
        mfc->extract_config_stream = ExtractConfigStreamMpeg4;
        mfc->next_frame = NextFrameMpeg4;
        mfc->MFC_DEC_GETCONF_STREAMINFO = MPEG4_DEC_GETCONF_STREAMINFO;
        mfc->MFC_DEC_GETCONF_PHYADDR_FRAM_BUF = MPEG4_DEC_GETCONF_PHYADDR_FRAM_BUF;
        mfc->SSBSIP_DEC_RET_OK = SSBSIP_MPEG4_DEC_RET_OK;
        break;
    case CODEC_ID_H263:
        //H.263
        mfc->frame_len = 200000;
        mfc->decode_init = SsbSipMPEG4DecodeInit;
        mfc->decode_exe = SsbSipMPEG4DecodeExe;
        mfc->decode_deinit = SsbSipMPEG4DecodeDeInit;
        mfc->decode_get_config  = SsbSipMPEG4DecodeGetConfig;
        mfc->decode_get_inbuf = SsbSipMPEG4DecodeGetInBuf;
        mfc->decode_get_outbuf = SsbSipMPEG4DecodeGetOutBuf;
        mfc->MFC_DEC_GETCONF_STREAMINFO = MPEG4_DEC_GETCONF_STREAMINFO;
        mfc->MFC_DEC_GETCONF_PHYADDR_FRAM_BUF = MPEG4_DEC_GETCONF_PHYADDR_FRAM_BUF;
        mfc->SSBSIP_DEC_RET_OK = SSBSIP_MPEG4_DEC_RET_OK;
        break;
    case CODEC_ID_VC1:
    case CODEC_ID_WMV3:
        //VC1
        mfc->frame_len = 200000;
        mfc->decode_init = SsbSipVC1DecodeInit;
        mfc->decode_exe = SsbSipVC1DecodeExe;
        mfc->decode_deinit = SsbSipVC1DecodeDeInit;
        mfc->decode_get_config  = SsbSipVC1DecodeGetConfig;
        mfc->decode_get_inbuf = SsbSipVC1DecodeGetInBuf;
        mfc->MFC_DEC_GETCONF_STREAMINFO = VC1_DEC_GETCONF_STREAMINFO;
        mfc->MFC_DEC_GETCONF_PHYADDR_FRAM_BUF = VC1_DEC_GETCONF_PHYADDR_FRAM_BUF;
        mfc->SSBSIP_DEC_RET_OK = SSBSIP_VC1_DEC_RET_OK;
        break;
    default:
        av_log(avctx, AV_LOG_ERROR, "Do not support this codec, exit!\n");
        return -1;

    }
    //creat new instance
    mfc->handle = mfc->decode_init();
    if(mfc->handle == NULL){
        av_log(avctx, AV_LOG_ERROR, "libmfc: MFC_Dec_Init Failed.\n");
        return -1;
    }
    
    //obtaining the input buffer
    mfc->stream_buf = mfc->decode_get_inbuf(mfc->handle, mfc->frame_len);
    if(mfc->stream_buf == NULL){
        av_log(avctx, AV_LOG_ERROR, "libmfc: mfc->decode_get_inbuf Failed.\n");
        mfc->decode_deinit(mfc->handle);
        return -1;
    }

#ifdef OUTPUT_TYPE_FILE
    mfc->out_fd = open("/fs/Desktop/Media/test_media/out.yuv", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if(mfc->out_fd < 0){
        av_log(avctx, AV_LOG_ERROR, "output file open failed\n");
        return -1;
    }
#endif //OUTPUT_TYPE_FILE
    return 0;
}

int 
MFC_decode_frame(AVCodecContext *avctx, void *data, int *data_size, AVPacket* avpkt)
{
#ifdef OUTPUT_TYPE_FILE  
    long yuv_len;
#endif

    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    AVCodec *codec = avctx->codec;
    MFCContext *mfc = avctx->priv_data;
    AVFrame *pict = data;
    static uint8_t length_size;
    
    if(buf_size == 0){
        //TODO:
        return 0;
    }
    
    if(!mfc->init){
        if(codec->id == CODEC_ID_MPEG4){
            rawbuf = buf;
            rawbuf_size = buf_size;
            if(avctx->extradata_size){
                rawbuf_size = buf_size + avctx->extradata_size;
                rawbuf = av_malloc(rawbuf_size);
                if (!rawbuf)
                    return AVERROR(ENOMEM);
                memcpy(rawbuf, avctx->extradata, avctx->extradata_size);
                memcpy(rawbuf+avctx->extradata_size, buf, buf_size);
            }            
            (mfc->file_strm).p_start = (mfc->file_strm).p_cur = (uint8_t *)rawbuf;
            (mfc->file_strm).p_end = (uint8_t *)(rawbuf + rawbuf_size);

            //FrameExtractor Initializaion
            mfc->frameex_ctx = FrameExtractorInit(FRAMEX_IN_TYPE_MEM, mfc->delimiter_type, mfc->delimiter_size, 1);  
            FrameExtractorFirst(mfc->frameex_ctx, &mfc->file_strm);

            //CONFIG stream extraction
            mfc->frame_len = mfc->extract_config_stream(mfc->frameex_ctx, &mfc->file_strm, mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
        }
        else if (codec->id == CODEC_ID_H264){
            if(avctx->extradata_size)
                length_size = (*(avctx->extradata+4) & 0x3) + 1;

            bsfc = av_bitstream_filter_init("h264_mp4toannexb");
            av_bitstream_filter_filter(bsfc, avctx, NULL, &rawbuf, &rawbuf_size, buf, buf_size, 0);
            (mfc->file_strm).p_start = (mfc->file_strm).p_cur = (uint8_t *)rawbuf;
            (mfc->file_strm).p_end = (uint8_t *)(rawbuf + rawbuf_size);
            
            mfc->frameex_ctx = FrameExtractorInit(FRAMEX_IN_TYPE_MEM, mfc->delimiter_type, mfc->delimiter_size, 1);  
            FrameExtractorFirst(mfc->frameex_ctx, &mfc->file_strm);
            mfc->frame_len = mfc->extract_config_stream(mfc->frameex_ctx, &mfc->file_strm, mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
        }
        else if(codec->id == CODEC_ID_WMV3){            
            (mfc->file_strm).p_start = (mfc->file_strm).p_cur = (uint8_t *)buf;
            (mfc->file_strm).p_end = (uint8_t *)(buf + buf_size);
            mfc->frame_len = ExtractConfigStreamVC1(&mfc->file_strm, mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
        }
        else if (codec->id  == CODEC_ID_H263 ){            
            (mfc->file_strm).p_start = (mfc->file_strm).p_cur = (uint8_t *)buf;
            (mfc->file_strm).p_end = (uint8_t *)(buf + buf_size);
            mfc->frame_len = ExtractConfigStreamH263(&mfc->file_strm, mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
        }

        av_log(avctx, AV_LOG_DEBUG, "LIBMFC: CONFIG stream length = %d | %d\n", mfc->frame_len, buf_size);

        //configuring the instance with the config stream
        if (mfc->decode_exe(mfc->handle, mfc->frame_len) != mfc->SSBSIP_DEC_RET_OK) {
            av_log(avctx, AV_LOG_ERROR, "LIBMFC: MFC Decoder Configuration Failed.\n");
            return -1;
        }
        
        //get stream information
        mfc->decode_get_config(mfc->handle, mfc->MFC_DEC_GETCONF_STREAMINFO, &mfc->stream_info);
	avctx->width  = (mfc->stream_info).width;
	avctx->height = (mfc->stream_info).height;
        av_log(avctx, AV_LOG_DEBUG, "<STREAMINFO> width=%d height=%d buf_width=%d buf_height = %d.\n", (mfc->stream_info).width, (mfc->stream_info).height, (mfc->stream_info).buf_width, (mfc->stream_info).buf_height);

        mfc->init = 1;
    }
    else{
        //next video stream        
        if(codec->id == CODEC_ID_MPEG4){
            (mfc->file_strm).p_start = (mfc->file_strm).p_cur = (uint8_t *)buf + 3;
            (mfc->file_strm).p_end = (uint8_t *)(buf + buf_size);
            mfc->frame_len = mfc->next_frame(mfc->frameex_ctx, &mfc->file_strm, mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
        }
        else if(codec->id == CODEC_ID_H264){            
            (mfc->file_strm).p_start = (mfc->file_strm).p_cur = (uint8_t *)buf + length_size;
            (mfc->file_strm).p_end = (uint8_t *)(buf + buf_size);
            mfc->frame_len = mfc->next_frame(mfc->frameex_ctx, &mfc->file_strm, mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
            //Ugly hack to get the right slice length!
            mfc->frame_len = buf_size;
        }
        else if(codec->id == CODEC_ID_WMV3){
            (mfc->file_strm).p_start = (mfc->file_strm).p_cur = (uint8_t *)buf;
            (mfc->file_strm).p_end = (uint8_t *)(buf + buf_size);
            mfc->frame_len = NextFrameVC1(&mfc->file_strm, mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
        }
        else if(codec->id == CODEC_ID_H263){
            (mfc->file_strm).p_start = (mfc->file_strm).p_cur = (uint8_t *)buf + 3;
            (mfc->file_strm).p_end = (uint8_t *)(buf + buf_size);
            mfc->frame_len = NextFrameH263(&mfc->file_strm, mfc->stream_buf, INPUT_BUFFER_SIZE, NULL);
        }

        if(mfc->frame_len < 4){
            av_log(avctx, AV_LOG_ERROR, "Extract stream wrong\n");
            return -1;
        }

        //av_log(avctx, AV_LOG_DEBUG, "LIBMFC: stream length = %d | %d\n", mfc->frame_len, buf_size);
    }

    //decode
    if (mfc->decode_exe(mfc->handle, mfc->frame_len) != mfc->SSBSIP_DEC_RET_OK){
        av_log(avctx, AV_LOG_ERROR, "Oh, no! decode wrong\n");
        return -1;
    }
    
    //obtaining the output buffer
    //virtual address
    mfc->yuv_buf[0] = mfc->decode_get_outbuf(mfc->handle, &mfc->yuv_buf[1]);
    pict->data[0] = (void*)mfc->yuv_buf[0];
    pict->data[1] = pict->data[0] +
	    ((mfc->stream_info).width * (mfc->stream_info).height);
    pict->data[2] = pict->data[1] +
	    ((mfc->stream_info).width * (mfc->stream_info).height >> 2);
    pict->linesize[0] = (mfc->stream_info).width;
    pict->linesize[2] = pict->linesize[1] = pict->linesize[0] >> 1;
    *data_size = sizeof(AVFrame);
    //physical address, reserved for pp!
    mfc->decode_get_config(mfc->handle, mfc->MFC_DEC_GETCONF_PHYADDR_FRAM_BUF, mfc->yuv_buf);
    phy_addr = (void*)mfc->yuv_buf[0];

        
#ifdef OUTPUT_TYPE_FILE  
    mfc->yuv_buf[0] = SsbSipMPEG4DecodeGetOutBuf(mfc->handle, &yuv_len);
    yuv_len = write(mfc->out_fd, (mfc->yuv_buf)[0], ((mfc->stream_info).width * (mfc->stream_info).height * 3) >> 1);
    if(yuv_len < ((mfc->stream_info).width * (mfc->stream_info).height * 3) >> 1)
       av_log(avctx, AV_LOG_ERROR, "write fail\n");
#endif //OUTPUT_TYPE_FILE
     return 0;
}

static av_cold int
MFC_decode_close(AVCodecContext *avctx)
{
    MFCContext *mfc = avctx->priv_data;
    mfc->decode_deinit(mfc->handle);
    if(avctx->extradata_size){
        if(avctx->codec->id == CODEC_ID_MPEG4 && rawbuf != NULL)
            av_free(rawbuf);
        if(avctx->codec->id == CODEC_ID_H264 && bsfc != NULL)
            av_bitstream_filter_close(bsfc);
    }
#ifdef OUTPUT_TYPE_FILE
    close(mfc->out_fd);
#endif
    av_log(avctx, AV_LOG_DEBUG, "LIBMFC: @@@ Program ends.\n");
    return 0;
}

#define MFC_CODEC(id_, name_, long_name_)          \
    AVCodec name_ ## _mfc_decoder = {              \
        .name = "libmfc_" #name_,                  \
        .type = CODEC_TYPE_VIDEO,                  \
        .id = id_,                                 \
        .init = MFC_decode_init,                   \
        .decode = MFC_decode_frame,                \
        .close  = MFC_decode_close,                \
        .priv_data_size = sizeof(MFCContext),      \
        .long_name = NULL_IF_CONFIG_SMALL(long_name_),  \
    }

MFC_CODEC(CODEC_ID_H264, h264, "LIBMFC H.264 / MPEG-4 part 10");
MFC_CODEC(CODEC_ID_MPEG4, mpeg4, "LIBMFC MPEG-4 part 2");
MFC_CODEC(CODEC_ID_H263, h263, "LIBMFC H.263");
MFC_CODEC(CODEC_ID_WMV3, vc1, "LIBMFC VC1");

#undef MFC_CODEC
