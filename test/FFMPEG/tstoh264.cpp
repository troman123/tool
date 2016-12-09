int main(int argc, char *argv[])
{
    AVFormatContext *pFormatCtx;
    int                         i, videoStream;
    int audioStream;
    AVPacket                packet;

    FILE *hVideo;
    FILE *hAudio;

    const char *input_file_name = "F:\\FFmpeg-full-SDK-3.2\\res\\src\\2.ts";// 2.ts为采用h264和mp3编码的文件
    const char *output_file_name = "F:\\FFmpeg-full-SDK-3.2\\res\\dst\\2.h264";
    const char *output_file_name2 = "F:\\FFmpeg-full-SDK-3.2\\res\\dst\\2.mp3";

    hVideo = fopen(output_file_name, "wb+");
    if(hVideo == NULL)
    {
             return 0;
    }

    hAudio = fopen(output_file_name2, "wb+");
    if(hAudio == NULL)
    {
             return 0;
    }

    // Register all formats and codecs
    av_register_all();

    // Open video file
    if(av_open_input_file(&pFormatCtx, input_file_name, NULL, 0, NULL)!=0)
            return -1; 

    // Retrieve stream information
    if(av_find_stream_info(pFormatCtx) < 0)
            return -1; 

    // Dump information about file onto standard error
    dump_format(pFormatCtx, 0, input_file_name, false);

    // Find the first video stream
    videoStream = -1;
    audioStream = -1;
    for(i = 0; i < pFormatCtx->nb_streams; i++)
    {
            if(pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
            {
                    videoStream = i;
            }
            else if(pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO)
            {
                    audioStream = i;
            }
    }
    if(videoStream == -1)
    {
            return -1; 
    }

    while(av_read_frame(pFormatCtx, &packet) >= 0)
    {
            if(packet.stream_index == videoStream)
            {
                    int len;
                    len = fwrite(packet.data, 1, packet.size, hVideo);
                    //fwrite(&packet.pts, 1, sizeof(int64_t), hVideo);
            }
            else if(packet.stream_index == audioStream)
            {
                    fwrite(packet.data, 1, packet.size, hAudio);
                    //fwrite(&packet.pts, 1, sizeof(int64_t), hAudio);
            }

            // Free the packet that was allocated by av_read_frame
            av_free_packet(&packet);
    }

    av_close_input_file(pFormatCtx);

    fclose(hVideo);
    fclose(hAudio);

    return 0;
}