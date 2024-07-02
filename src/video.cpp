#include "video.h"
#include <math.h>
#include <thread>
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
    qDebug() << "Format " << formatContext->iformat->long_name << " duration " << formatContext->duration << " us";

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

bool Video::decodeVideo(int streamIndex, int64_t frameNumber, VideoFrame &videoFrame){

    AVStream* stream = formatContext->streams[videoStreamIndexes[streamIndex]];
    double framerate = stream->avg_frame_rate.num / (double)stream->avg_frame_rate.den;
    int64_t pts = (int64_t)((double)frameNumber * (double)stream->time_base.den / (double)((double)framerate * (double)stream->time_base.num));

    //qDebug()<<"seek for " << pts;
    //qDebug()<<"seek for adjusted" << pts + stream->start_time;
    streamIndex = videoStreamIndexes[streamIndex];

    AVCodecContext* codecCtx = codecContexts[streamIndex];

    //qDebug() << "starting at " <<stream->start_time;
    pts += stream->start_time;
    avcodec_flush_buffers(codecCtx);
    av_seek_frame(formatContext, streamIndex, pts, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);


    bool found = false;
    int responce;

    while(!found){
        responce = av_read_frame(formatContext,packet);

        if(responce>0){
            //av_packet_unref(packet);
            continue;
        }


        if(packet->stream_index != streamIndex){
            //av_packet_unref(packet);
            continue;
        }


        responce = avcodec_send_packet(codecCtx,packet);

        if(responce<0){
            qDebug("Failed to read packet\n");
        }

        responce = avcodec_receive_frame(codecCtx,frame);
        if(responce == AVERROR(EAGAIN) || responce == AVERROR(EOF)){
            if(responce == AVERROR(EOF) ){
                avcodec_send_packet(codecCtx, nullptr);
                continue;
            }
            //av_packet_unref(packet);
            //av_frame_unref(frame);
            continue;
        }


        if (frame->pts >=pts) {
            found = true;
            //av_packet_unref(packet);
            break;
        }

        else if(responce < 0){

            qDebug("Failed to decode packet: %s \n", av_make_error(responce));
            return false;

        }

        if(found)
            break;


        //av_packet_unref(packet);

    }


    if (!found) {
        qDebug("Frame with PTS %llu not found", pts);
        av_packet_unref(packet);
        av_frame_unref(frame);
        return false;
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
     /* Higher bitdepth videos */

    uint8_t* frameBuffer = new uint8_t [frame->width * frame->height * 4 ]; //multiply by 4 for the 4 channels R G B A
    uint8_t* output[4] = {frameBuffer,NULL,NULL,NULL};
    int outputLinesize [4] = {frame->width * 4,0,0,0};

    sws_scale(scaler,frame->data,frame->linesize,0,frame->height,output,outputLinesize);

    videoFrame.frameData = output[0];
    videoFrame.width = frame->width;
    videoFrame.height = frame->height;
    videoFrame.linesize = outputLinesize[0];
    //videoFrame->format =
    av_frame_unref(frame);
    av_packet_unref(packet);


    return true;
}

std::vector<AudioFrame> Video::decodeAudio(int streamIndex, double startTime, std::vector<AudioFrame> &audioFrames){

    AVCodecContext* codecCtx = codecContexts[audioStreamIndexes[streamIndex]];
    streamIndex = audioStreamIndexes[streamIndex];
    AVStream* stream = formatContext->streams[streamIndex];


    int64_t pts = (startTime-(double)m_bufferPaddingTime) * stream->time_base.den/(double)stream->time_base.num;
    if(pts<0)
        pts=0;
    int64_t nextpts = (startTime+ (double)(m_bufferTime + m_bufferPaddingTime)) * stream->time_base.den/(double)stream->time_base.num;

    av_seek_frame(formatContext,streamIndex,pts, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codecCtx);

    int responce;
    bool found = false;


    while(!found && av_read_frame(formatContext,packet)>=0){

        if (packet->stream_index != streamIndex ){
            av_packet_unref(packet);
            continue;
        }

        //codecCtx = codecContexts[streamIndexes[packet->stream_index]];


        responce = avcodec_send_packet(codecCtx,packet);
        if(responce<0){
            qDebug("Failed to decode packet\n");
            return audioFrames;
        }

        responce = avcodec_receive_frame(codecCtx,frame);

        if(responce == AVERROR(EAGAIN) || responce == AVERROR(EOF)){

            av_packet_unref(packet);
            continue;
        }


        if(frame->pts<pts){
            //qDebug()<< "packet pts: " << packet->pts;

            av_packet_unref(packet);

            av_frame_unref(frame);
            continue;
        }
        if(frame->pts>nextpts){
            //qDebug()<< "packet pts: " << packet->pts;

            av_packet_unref(packet);
            av_frame_unref(frame);
            return audioFrames;
        }




        //qDebug()<<"framePts: " << frame->pts;
        //qDebug()<< "packet pts: " << packet->pts;

        SwrContext* sampler = resampleContexts[streamIndexes[packet->stream_index]];

        int src_nb_samples = frame->nb_samples, dst_nb_samples = frame->nb_samples;
        enum AVSampleFormat src_sample_fmt = codecCtx->sample_fmt, dst_sample_fmt = av_get_packed_sample_fmt(static_cast<AVSampleFormat>(codecCtx->sample_fmt));
        int src_nb_channels = frame->ch_layout.nb_channels, dst_nb_channels = frame->ch_layout.nb_channels;

       // swr_alloc_set_opts2(&sampler,
         //                   &frame->ch_layout,dst_sample_fmt,dst_nb_samples,&frame->ch_layout,src_sample_fmt,src_nb_samples,0,nullptr);
        //swr_init(sampler);
        uint8_t* output_buffer;

        int output_buffer_size = av_samples_get_buffer_size(nullptr, dst_nb_channels,dst_nb_samples, dst_sample_fmt, 0);
        output_buffer = new uint8_t[output_buffer_size];

        swr_convert(sampler,&output_buffer,dst_nb_samples,(const uint8_t **)frame->data,src_nb_samples);

        AudioFrame audioFrame;
        audioFrame.frameData= output_buffer;
        audioFrame.frameSize = output_buffer_size;
        audioFrame.pts = frame->pts;
        audioFrames.push_back(audioFrame);

        av_packet_unref(packet);
        av_frame_unref(frame);



    }

    return audioFrames;
}

bool Video::isAudioBuffered(int streamIndex, double time){
    streamIndex = audioStreamIndexes[streamIndex];
    AVStream* stream = formatContext->streams[streamIndex];
    const AVRational &timebase = stream->time_base;
    int64_t pts = (time) * timebase.den/timebase.num;
    uint64_t startPTS = m_audioBuffer.startTime * timebase.den/timebase.num;
    uint64_t endPTS = (m_audioBuffer.endTime) * timebase.den/timebase.num;

    if(pts>=startPTS && pts<= endPTS)
        return true;

    return false;
}

void Video::bufferAudio(int streamIndex, double start){
    std::vector<AudioFrame> frames;

    m_audioBuffer = AudioBuffer();

    AVStream* stream = formatContext->streams[videoStreamIndexes[0]];
    double framerate = stream->avg_frame_rate.num / stream->avg_frame_rate.den;
    double startTime = start;

    decodeAudio(streamIndex,startTime,frames);

    m_audioBuffer.startTime = startTime;
    m_audioBuffer.endTime = startTime + m_bufferTime;

    Video::moveToAudioBuffer(m_audioBuffer,frames,m_bufferPaddingTime);


}

void Video::moveToAudioBuffer(AudioBuffer &buffer, std::vector<AudioFrame> &frames, int padding){
    if(frames.empty())
        return;

    int bufferSize = 0;
    buffer.startPTS = frames.cbegin()->pts;
    buffer.endPTS = frames.cend()->pts;

    for(const AudioFrame &f : frames){
        buffer.size += f.frameSize;
    }

    buffer.data = new uint8_t[buffer.size];
    uint8_t* ptr = buffer.data;

    buffer.frameCount =0;

    for(int i = 0; i<frames.size(); i++){
        memcpy(ptr,frames[i].frameData,frames[i].frameSize);
        ptr+=frames[i].frameSize;
        buffer.frameCount++;
        frames[i].clean();
    }

    buffer.paddingFrameCount = padding;

}

void Video::getAudio(int streamIndex, int64_t frameNumber, double start, double end, Audio &audio){

    if(!isAudioBuffered(streamIndex,start))
        bufferAudio(streamIndex,start);

    AVCodecContext* codecCtx = codecContexts[audioStreamIndexes[streamIndex]];
    streamIndex = audioStreamIndexes[streamIndex];
    AVStream* stream = formatContext->streams[videoStreamIndexes[0]];
    double framerate = stream->avg_frame_rate.num / stream->avg_frame_rate.den;

    AVRational timebase = formatContext->streams[streamIndex]->time_base;
    int sampleRate = codecCtx->sample_rate;
    int bytesPerSample = av_get_bytes_per_sample(av_get_packed_sample_fmt(static_cast<AVSampleFormat>(codecCtx->sample_fmt)));
    double timebaseFactor = double(timebase.den/timebase.num);

    int64_t startPtsOffset = ((double)((double)start) * timebaseFactor) - m_audioBuffer.startPTS;
    int64_t startSampleOffset = (double)((double)startPtsOffset/ timebaseFactor ) *sampleRate ;
    int64_t startByteOffset = startSampleOffset* codecCtx->ch_layout.nb_channels *bytesPerSample;

    int64_t endPtsOffset = ( (double)((double)end) * timebaseFactor) - m_audioBuffer.startPTS ;
    int64_t endSampleOffset = (double)((double)endPtsOffset/ timebaseFactor) *sampleRate;
    int64_t endByteOffset = endSampleOffset * codecCtx->ch_layout.nb_channels  *bytesPerSample;

    endByteOffset-=startByteOffset;

    if(startByteOffset+endByteOffset>m_audioBuffer.size || startByteOffset<0 || endByteOffset<0)
        return;

    audio.data = new uint8_t[endByteOffset];
    audio.size = endByteOffset;

    memcpy(audio.data,m_audioBuffer.data+startByteOffset,endByteOffset);

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

bool Video::getVideoStreamInfo(int streamIndex, VideoStreamInfo &info)
{
    int trueIndex =  videoStreamIndexes[streamIndex];
    AVStream* stream = formatContext->streams[trueIndex];
    if(!stream){
        qDebug()<<"invalid stream index";
        return false;
    }
    AVCodecContext* codecCtx = codecContexts[streamIndexes[videoStreamIndexes[streamIndex]]];
    info.frameRate = stream->avg_frame_rate.num;
    info.width = codecCtx->width;
    info.height = codecCtx->height;
    info.pixFmt = codecCtx->pix_fmt;
    info.frameCount = stream->nb_frames;
    if(info.frameCount<=0)
        info.frameCount=frameCount;

    return true;
}

bool Video::getVideoFileInfo(VideoFileInfo &info)
{
    if(!formatContext)
        return false;
    info.numberStreams = formatContext->nb_streams;
    info.numberAudioStreams = numAudioStreams;
    info.numberVideoStreams = numVideoStreams;
    return true;
}


quint64 Video::assignStreamId(int streamIndex, AVFormatContext *format){
    AVCodecParameters* params = format->streams[streamIndex]->codecpar;
    AVMediaType type = params->codec_type;
    if(type==AVMEDIA_TYPE_VIDEO){

        numVideoStreams++;
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
    codecCtx->thread_count = std::thread::hardware_concurrency();
    if (codec->capabilities & AV_CODEC_CAP_FRAME_THREADS)
        codecCtx->thread_type = FF_THREAD_FRAME;
    else if (codec->capabilities & AV_CODEC_CAP_SLICE_THREADS)
        codecCtx->thread_type = FF_THREAD_SLICE;
    else
        codecCtx->thread_count = 1;
    avcodec_open2(codecCtx,codec,NULL);
    quint64 id = nextId++;
    codecContexts[id] = codecCtx;
    streamIndexes[streamIndex]=id;


    if(type==AVMEDIA_TYPE_VIDEO){
        SwsContext* scaler = sws_getContext(params->width,params->height,static_cast<AVPixelFormat>(params->format),params->width,params->height,AV_PIX_FMT_RGBA,SWS_BICUBLIN,NULL,NULL,NULL);
        scalerContexts[id] = scaler;
        width = params->width;
        height = params->height;
        m_frameRate = formatContext->streams[videoStreamIndexes[0]]->avg_frame_rate.num / (double)(formatContext->streams[videoStreamIndexes[0]]->avg_frame_rate.den);
        startTime = formatContext->streams[videoStreamIndexes[0]]->start_time;
        frameCount = formatContext->streams[videoStreamIndexes[0]]->nb_frames;
        duration = (double)formatContext->streams[videoStreamIndexes[0]]->duration*(double)(formatContext->streams[videoStreamIndexes[0]]->time_base.num/formatContext->streams[videoStreamIndexes[0]]->time_base.den);
        if(duration<=0){
            duration = formatContext->duration /(double)1e6;
            frameCount = std::floor(duration*m_frameRate);
        }
        AVStream* ABC = formatContext->streams[videoStreamIndexes[0]];
        int a=2;
    }
    else{
        SwrContext* resampler = nullptr;
        int src_nb_samples = params->sample_rate, dst_nb_samples = params->sample_rate;
        enum AVSampleFormat src_sample_fmt = codecCtx->sample_fmt, dst_sample_fmt = av_get_packed_sample_fmt(static_cast<AVSampleFormat>(codecCtx->sample_fmt));
        //int src_nb_channels = frame->ch_layout.nb_channels, dst_nb_channels = frame->ch_layout.nb_channels;

        swr_alloc_set_opts2(&resampler,
                            &params->ch_layout,dst_sample_fmt,dst_nb_samples,&params->ch_layout,src_sample_fmt,src_nb_samples,0,nullptr);
        //swr_init(sampler);
        //swr_alloc_set_opts2(&resampler,
          //                  &params->ch_layout,AV_SAMPLE_FMT_S16,codecCtx->sample_rate,&params->ch_layout,codecCtx->sample_fmt,codecCtx->sample_rate,0,nullptr);

        if ((swr_init(resampler)) < 0)
            qDebug()<<"Failed to initialize the resampling context";
        resampleContexts[id] = resampler;
    }

    return id;
}
