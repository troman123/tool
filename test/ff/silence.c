int frame_size_out_encode = out_fmt_ctx->streams[out_stream->index]->codec->frame_size;
if(!frame_size_out_encode)
	frame_size_out_encode = put_into_fifo->nb_samples;



确定采样长度，先取得编码器frame_size，如果该值是0，比如pcm，则用输入Frame的nb_samples 作为采样长度。

AVCodecContext *acout = out_fmt_ctx->streams[0]->codec;	
AVFrame *frame_fifo;  
frame_fifo = av_frame_alloc();
frame_fifo->nb_samples     = frame_size_out_encode;
frame_fifo->channel_layout = acout->channel_layout;  
frame_fifo->channels = av_get_channel_layout_nb_channels(frame_fifo->channel_layout);
frame_fifo->format         = acout->sample_fmt;  
frame_fifo->sample_rate    = acout->sample_rate;  

av_frame_get_buffer(frame_fifo, 0);
av_samples_set_silence(frame_fifo->data, 0,
	frame_fifo->nb_samples, frame_fifo->channels,
	(AVSampleFormat)frame_fifo->format);


按channels，format等参数申请缓存

av_init_packet(&pkt_out);
int ret = avcodec_encode_audio2(out_fmt_ctx->streams[0]->codec,
	&pkt_out, frame_fifo, &got_picture);
if (got_picture ) 
{
	pkt_out.stream_index = out_stream->index;
	pkt_out.dts = av_rescale_q_rnd(pkt_out.dts,
		out_fmt_ctx->streams[out_stream->index]->codec->time_base,
		out_fmt_ctx->streams[out_stream->index]->time_base,
		(AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));

	pkt_out.pts = av_rescale_q_rnd(pkt_out.pts,
		out_fmt_ctx->streams[out_stream->index]->codec->time_base,
		out_fmt_ctx->streams[out_stream->index]->time_base,
		(AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));

	pkt_out.duration = av_rescale_q(pkt_out.duration,
		out_fmt_ctx->streams[out_stream->index]->codec->time_base,
		out_fmt_ctx->streams[out_stream->index]->time_base);						 

	av_log(NULL, AV_LOG_DEBUG, "Muxing frame\n");
	/* mux encoded frame */
	ret = av_interleaved_write_frame(out_fmt_ctx, &pkt_out);
	av_free_packet(&pkt_out);
	if (ret < 0)
	{
		printf("write a null frame failed!\n");
		break;
	}
	printf("success write a null frame:index %d\n", frame_index++);

	duration_sum += frame_size_out_encode;
	if(duration_sum * av_q2d(out_fmt_ctx->streams[out_stream->index]->codec->time_base) >= g_offset)
		break;
}


编码，写入AVPacket，duration_sum记录总时长的信息，g_offset是静音时常
