#include "video.h"


const char* av_make_error(int errnum) {
    static char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}



Video::Video(const char *path) : m_path(path){


}

bool Video::open(){
    formatContext = avformat_alloc_context();
    if(avformat_open_input(&formatContext,m_path,NULL,NULL)<0){
        qDebug()<<"Could not open source file";
        return false;
    }
    qDebug() << "Format " >> formatContext->iformat->long_name << " duration " << formatContext->duration << " us";

    if(avformat_find_stream_info(formatContext,NULL) <0){
        qDebug()<<"Could not find stream info";
        return false;
    }

    AVCodecParameters *codecParameters;
    for (int i = 0; i < formatContext->nb_streams; i++)
    {
        assignStreamId(i,formatContext);
    }

    packet = av_packet_alloc();

    if(!packet){
        qDebug("error could not allocate packet\n");
        return false;
    }


    frame = av_frame_alloc();

    if(!frame){
        qDebug("err could not allocate frame\n");
        return false;
    }

    return true;
}

bool Video::decodeVideo(int streamIndex, int frameNumber, VideoFrame *videoFrame){

    AVStream* stream = formatContext->streams[videoStreamIndexes[streamIndex]];
    double framerate = stream->avg_frame_rate.num / stream->avg_frame_rate.den;
    uint64_t pts = (int64_t)(frameNumber * (double)stream->time_base.den / ((double)framerate * stream->time_base.num));

    streamIndex = videoStreamIndexes[streamIndex];

    av_seek_frame(formatContext,streamIndex,pts, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
    bool found = false;
    int responce;


    while(!found && av_read_frame(formatContext,packet) >= 0){
        if(packet->stream_index != streamIndex){
            av_packet_unref(packet);
            continue;
        }

        AVCodecContext* codecCtx = codecContexts[videoStreamIndexes[streamIndex]];

        responce = avcodec_send_packet(codecCtx,packet);

        if(responce<0){
            qDebug("Failed to decode packet\n");
            return false;
        }

        responce = avcodec_receive_frame(codecCtx,frame);

        if(responce == AVERROR(EAGAIN) || responce == AVERROR(EOF)){

            av_packet_unref(packet);
            continue;
        }

        else if (frame->best_effort_timestamp == pts) {
            found = true;
            av_packet_unref(packet);
            break;
        }

        else if(responce < 0){

            qDebug("Failed to decode packet: %s \n", av_make_error(responce));
            return false;

        }

        //av_packet_unref(state->packet);
        //break;

    }


    pts = frame->pts;


    SwsContext* scaler = scalerContexts[streamIndexes[streamIndex]];
    if(!scaler){
        /* TO DO */
        //initializeScaler();

    }




    if (!scaler) {
        qDebug("Couldn't initialize sw scaler\n");
        return false;
    }


    //convert YUV data to RGBA
    /* TO DO */


    //uint8_t* output[4] = {frameBuffer,NULL,NULL,NULL};
    int outputLinesize [4] = {frame->width * 4,0,0,0};

    //sws_scale(scaler,frame->data,frame->linesize,0,frame->height,output,outputLinesize);

   // videoFrame->frameData = output;
    videoFrame->width = frame->width;
    videoFrame->height = frame->height;
    //videoFrame->linesize = frame->linesize;
    //videoFrame->format =

    return true;
}

bool Video::decodeAudio(int streamIndex, int frameNnumber, AudioFrame &audioFrame){
    AVCodecContext* codecCtx = codecContexts[audioStreamIndexes[streamIndex]];
    streamIndex = audioStreamIndexes[streamIndex];

    uint64_t pts = frameNnumber * codecCtx->frame_size;
    av_seek_frame(formatContext,streamIndex,pts,  AVSEEK_FLAG_FRAME);
    //avcodec_flush_buffers(codecCtx);

    int responce;
    bool found = false;


    while(!found && av_read_frame(formatContext,packet)>=0){

        if (packet->stream_index != audioStreamIndexes.at(streamIndex) ){
            av_packet_unref(packet);
            continue;
        }

        codecCtx = codecContexts[streamIndexes[packet->stream_index]];


        responce = avcodec_send_packet(codecCtx,packet);
        if(responce<0){
            qDebug("Failed to decode packet\n");
            return false;
        }

        responce = avcodec_receive_frame(codecCtx,frame);

        if(responce == AVERROR(EAGAIN) || responce == AVERROR(EOF)){

            av_packet_unref(packet);
            continue;
        }


        if(frame->best_effort_timestamp!=pts){
            av_packet_unref(packet);
            //av_frame_unref(frame);
            continue;
        }

        SwrContext* sampler = resampleContexts[streamIndexes[packet->stream_index]];

        int src_nb_samples = frame->nb_samples, dst_nb_samples = frame->nb_samples;
        enum AVSampleFormat src_sample_fmt = codecCtx->sample_fmt, dst_sample_fmt = av_get_packed_sample_fmt(static_cast<AVSampleFormat>(codecCtx->sample_fmt));
        int src_nb_channels = frame->ch_layout.nb_channels, dst_nb_channels = frame->ch_layout.nb_channels;

        swr_alloc_set_opts2(&sampler,
                            &frame->ch_layout,dst_sample_fmt,dst_nb_samples,&frame->ch_layout,src_sample_fmt,src_nb_samples,0,nullptr);
        swr_init(sampler);
        uint8_t* output_buffer;

        int output_buffer_size = av_samples_get_buffer_size(nullptr, dst_nb_channels,dst_nb_samples, dst_sample_fmt, 0);
        output_buffer = new uint8_t[output_buffer_size];

        swr_convert(sampler,&output_buffer,dst_nb_samples,(const uint8_t **)frame->data,src_nb_samples);

        av_packet_unref(packet);
        av_frame_unref(frame);
        audioFrame.frameData= output_buffer;
        audioFrame.frameSize = output_buffer_size;
        return true ;

    }

    return false;
}

bool Video::getAudioStreamInfo(int streamIndex, AudioStreamInfo &info)
{
    int trueIndex =  audioStreamIndexes[streamIndex];
    AVStream* stream = formatContext->streams[trueIndex];
    if(!stream){
        qDebug()<<"invalid stream index";
        return false;
    }
    AVCodecContext* codecCtx = codecContexts[streamIndexes[audioStreamIndexes[streamIndex]]];
    info.channelCount = codecCtx->ch_layout.nb_channels;
    info.format = codecCtx->sample_fmt;
    info.sampleRate = codecCtx->sample_rate;
    info.packedFormat = av_get_packed_sample_fmt(static_cast<AVSampleFormat>(codecCtx->sample_fmt));

    return true;
}

quint64 Video::assignStreamId(int streamIndex, AVFormatContext *format){
    AVCodecParameters* params = format->streams[streamIndex]->codecpar;
    AVMediaType type = params->codec_type;
    if(type==AVMEDIA_TYPE_VIDEO){
        numVidesStreams++;
        videoStreamIndexes.push_back(streamIndex);

    }else if(type==AVMEDIA_TYPE_AUDIO){

        numAudioStreams++;
        audioStreamIndexes.push_back(streamIndex);

    }else{
        return -1;
    }

    AVCodecContext* codecCtx;

    AVCodec* codec = const_cast<AVCodec*>(avcodec_find_decoder(params->codec_id));
    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx,params);
    avcodec_open2(codecCtx,codec,NULL);
    quint64 id = nextId++;
    codecContexts[id] = codecCtx;
    streamIndexes[streamIndex]=id;


    if(type==AVMEDIA_TYPE_VIDEO){
        SwsContext* scaler = sws_getContext(params->width,params->height,static_cast<AVPixelFormat>(params->format),params->width,params->height,AV_PIX_FMT_RGB0,SWS_BICUBLIN,NULL,NULL,NULL);
        scalerContexts[id] = scaler;
    }
    else{
        SwrContext* resampler = nullptr;
        swr_alloc_set_opts2(&resampler,
                            &params->ch_layout,AV_SAMPLE_FMT_S16,codecCtx->sample_rate,&params->ch_layout,codecCtx->sample_fmt,codecCtx->sample_rate,0,nullptr);

        if ((swr_init(resampler)) < 0)
            qDebug()<<"Failed to initialize the resampling context";
        resampleContexts[id] = resampler;
    }

    return id;
}