#include "stdafx.h"
extern "C"
{
    #include <libavutil/motion_vector.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <iostream>
using namespace std;
// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
    #define av_frame_alloc avcodec_alloc_frame
    #define av_frame_free avcodec_free_frame
#endif

class MotionDetector
{
private:
    int   video_frame_count = 0;

    int print_motion_vectors_data(const AVMotionVector* mv, int video_frame_count)
    {
        printf("| #:%d | p/f:%2d | %2d x %2d | src:(%4d,%4d) | dst:(%4d,%4d) | dx:%4d | dy:%4d | motion_x:%4d | motion_y:%4d | motion_scale:%4d | 0x%4d |  \n",
            video_frame_count, mv->source, mv->w, mv->h, mv->src_x, mv->src_y, mv->dst_x, mv->dst_y, mv->dst_x - mv->src_x, mv->dst_y - mv->src_y, mv->motion_x,
            mv->motion_y, mv->motion_scale, mv->flags);
        return 0;
    }

    void write_mv_in_file(const AVMotionVector* mvs, FILE* file, unsigned int sz)
    {
        for (unsigned int i = 0; i < sz / sizeof(*mvs); i++) {
            const AVMotionVector* mv = &mvs[i];

            //print_motion_vectors_data(mv, video_frame_count);

            if (i == (sz / sizeof(*mvs)) - 1)
            {
                fprintf(file, "\t{\n");
                fprintf(file, "\t\t\"source\" : %d,\n", mv->source);
                fprintf(file, "\t\t\"width\" : %d,\n", mv->w);
                fprintf(file, "\t\t\"height\" : %d,\n", mv->h);
                if (mv->source < 0)
                {
                    fprintf(file, "\t\t\"src_x\" : %d,\n", ((mv->src_x) / abs(mv->source)));
                    fprintf(file, "\t\t\"src_y\" : %d,\n", (mv->src_y) / abs(mv->source));
                    fprintf(file, "\t\t\"dst_x\" : %d,\n", (mv->dst_x / abs(mv->source)));
                    fprintf(file, "\t\t\"dst_y\" : %d,\n", (mv->dst_y / abs(mv->source)));
                    fprintf(file, "\t\t\"dx\" : %d,\n", ((mv->dst_x - mv->src_x) / abs(mv->source)));
                    fprintf(file, "\t\t\"dy\" : %d\n", ((mv->dst_y - mv->src_y) / abs(mv->source)));
                }
                else
                {
                    fprintf(file, "\t\t\"src_x\" : %d,\n", (mv->dst_x / abs(mv->source)));
                    fprintf(file, "\t\t\"src_y\" : %d,\n", (mv->dst_y / abs(mv->source)));
                    fprintf(file, "\t\t\"dst_x\" : %d,\n", (mv->src_x / abs(mv->source)));
                    fprintf(file, "\t\t\"dst_y\" : %d,\n", (mv->src_y / abs(mv->source)));
                    fprintf(file, "\t\t\"dx\" : %d,\n", ((mv->src_x - mv->dst_x) / abs(mv->source)));
                    fprintf(file, "\t\t\"dy\" : %d\n", ((mv->src_y - mv->dst_y) / abs(mv->source)));
                }
                fprintf(file, "\t}\n");
            }
            else
            {
                fprintf(file, "\t{\n");
                fprintf(file, "\t\t\"source\" : %d,\n", mv->source);
                fprintf(file, "\t\t\"width\" : %d,\n", mv->w);
                fprintf(file, "\t\t\"height\" : %d,\n", mv->h);
                if (mv->source < 0)
                {
                    fprintf(file, "\t\t\"src_x\" : %d,\n", (mv->src_x / abs(mv->source)));
                    fprintf(file, "\t\t\"src_y\" : %d,\n", (mv->src_y / abs(mv->source)));
                    fprintf(file, "\t\t\"dst_x\" : %d,\n", (mv->dst_x / abs(mv->source)));
                    fprintf(file, "\t\t\"dst_y\" : %d,\n", (mv->dst_y / abs(mv->source)));
                    fprintf(file, "\t\t\"dx\" : %d,\n", ((mv->dst_x - mv->src_x) / abs(mv->source)));
                    fprintf(file, "\t\t\"dy\" : %d\n", ((mv->dst_y - mv->src_y) / abs(mv->source)));
                }
                else
                {
                    fprintf(file, "\t\t\"src_x\" : %d,\n", (mv->dst_x / abs(mv->source)));
                    fprintf(file, "\t\t\"src_y\" : %d,\n", (mv->dst_y / abs(mv->source)));
                    fprintf(file, "\t\t\"dst_x\" : %d,\n", (mv->src_x / abs(mv->source)));
                    fprintf(file, "\t\t\"dst_y\" : %d,\n", (mv->src_y / abs(mv->source)));
                    fprintf(file, "\t\t\"dx\" : %d,\n", ((mv->src_x - mv->dst_x) / abs(mv->source)));
                    fprintf(file, "\t\t\"dy\" : %d\n", ((mv->src_y - mv->dst_y) / abs(mv->source)));
                }
                fprintf(file, "\t},\n");
            }
        }
    }

    unsigned int calculate_motion_value(const AVMotionVector* mvs, unsigned int sz)
    {
        unsigned int value = 0;
        for (unsigned int i = 0; i < sz / sizeof(*mvs); i++) 
        {
            const AVMotionVector* mv = &mvs[i];
            value += sqrt( pow((mv->src_x - mv->dst_x),2) + pow((mv->src_y - mv->dst_y),2) );
        }
        return value;
    }

    unsigned int calculate_motion_value_for_frame(AVFrame* pFrame)
    {
        int i;
        AVFrameSideData* sd;
        FILE* file = NULL;
        char szFileName[255] = { 0 };
        video_frame_count++;
        
        /*
        sprintf_s(szFileName, "Frame-mv-%d.json", video_frame_count);
        fopen_s(&file, szFileName, "w");
        if (file == NULL)
        {
            fprintf(stderr, "Couldn't open file for reading\n");
            exit(1);
        }
        fprintf(file, "[\n");
        */

        sd = av_frame_get_side_data(pFrame, AV_FRAME_DATA_MOTION_VECTORS);
        unsigned int motion_value = 0;
        if (sd) {
            const AVMotionVector* mvs = (const AVMotionVector*)sd->data;
            //write_mv_in_file(mvs, file, sd->size);
            motion_value = calculate_motion_value(mvs, sd->size);
        }

        /*
        fprintf(file, "]\n");
        fclose(file);
        */

        printf("\rTotal Processed Frames:%d  , ", video_frame_count);
        fflush(stdout);
        av_frame_unref(pFrame);
        return motion_value;
    }

public: 
     MotionDetector(){}
    ~MotionDetector(){}
    int motion_detector(const AVPacket* pkt, AVCodecContext* pCodecCtx, AVFrame* pFrame)
    {
        int ret = avcodec_send_packet(pCodecCtx, pkt);
        if (ret < 0) {
            fprintf(stderr, "Error while sending a packet to the decoder: \n");
            return ret;
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_frame(pCodecCtx, pFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            else if (ret < 0) {
                fprintf(stderr, "Error while receiving a frame from the decoder:\n");
                return ret;
            }

            if (ret >= 0)
            {
                cout<<" Motion value = "<<calculate_motion_value_for_frame(pFrame)<<endl;
            }
        }
        return 0;
    }
};

int main(int argc, char **argv)
{
    int ret = 0;
    AVPacket pkt = { 0 };
    struct stat sb;
    const char* src_filename = NULL;
    char* videopath = argv[1];
    src_filename = videopath;

    AVStream* st;
    AVCodecContext* dec_ctx = NULL;
    AVCodec* dec = NULL;
    AVDictionary* opts = NULL;
    int    video_stream_idx = -1;

    AVFormatContext* pFormatCtx = NULL;
    AVCodecContext* pCodecCtx = NULL;
    AVFrame* pFrame = NULL;

    av_register_all();

    if (avformat_open_input(&pFormatCtx, videopath, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        exit(1);
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
            av_get_media_type_string(AVMEDIA_TYPE_VIDEO), src_filename);
        return ret;
    }
    else
    {
        int stream_idx = ret;
        st = pFormatCtx->streams[stream_idx];

        dec_ctx = avcodec_alloc_context3(dec);
        if (!dec_ctx) {
            fprintf(stderr, "Failed to allocate codec\n");
            return AVERROR(EINVAL);
        }

        ret = avcodec_parameters_to_context(dec_ctx, st->codecpar);
        if (ret < 0) {
            fprintf(stderr, "Failed to copy codec parameters to codec context\n");
            return ret;
        }

        /* Init the video decoder */
        av_dict_set(&opts, "flags2", "+export_mvs", 0);
        if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            return ret;
        }

        video_stream_idx = stream_idx;
        pCodecCtx = dec_ctx;
    }

    pFrame = av_frame_alloc();
    if (!pFrame) {
        fprintf(stderr, "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        //goto end;
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        av_frame_free(&pFrame);
    }

    /* read frames from the file */

    MotionDetector md_obj;

    while (av_read_frame(pFormatCtx, &pkt) >= 0)
    {
        if (pkt.stream_index == video_stream_idx) 
        {
            ret = md_obj.motion_detector(&pkt, pCodecCtx,pFrame);
        }
        av_packet_unref(&pkt);
        if (ret < 0)
            break;
    }

    /* flush cached frames */
    md_obj.motion_detector(NULL, pCodecCtx, pFrame);

//end:
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
    av_frame_free(&pFrame);
    return 0;
}

