#include "XVideoWriter.h"

#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}


#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")


using namespace std;
class CXVideoWriter : public XVideoWriter
{
public:
	bool Init(const char *file) override
	{
		Close();

		// 封装文件输出的上下文
		avformat_alloc_output_context2(&ic, nullptr, nullptr, file);
		if (!ic)
		{
			cerr << "avformat_alloc_output_context2 failed!" << endl;
			return false;
		}

		fileName = file;

		return true;
	}

	void Close() override
	{
		if (ic)
		{
			avformat_close_input(&ic);
		}

		if (vc)
		{
			// 必须先关闭，再释放
			avcodec_close(vc);
			avcodec_free_context(&vc);
		}

		if (ac)
		{
			avcodec_close(ac);
			avcodec_free_context(&ac);
		}

		if (vsc)
		{
			sws_freeContext(vsc);
			vsc = nullptr;
		}

		if (asc)
		{
			swr_free(&asc);
		}

		if (yuv)
		{
			av_frame_free(&yuv);
		}

		if (pcm)
		{
			av_frame_free(&pcm);
		}
	}

	bool AddVideoStream() override
	{
		if (!ic)
		{
			return false;
		}
		// 1. 视频编码器创建
		AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec)
		{
			cerr << "avcodec_find_encoder AV_CODEC_ID_H264 failed!" << endl;
			return false;
		}

		vc = avcodec_alloc_context3(codec);
		if (!vc)
		{
			cerr << "avcodec_alloc_context3 AV_CODEC_ID_H264 failed!" << endl;
			return false;
		}
		// 比特率，压缩后每秒大小
		vc->bit_rate = vBitRate;
		vc->width = outWidth;
		vc->height = outHeight;

		// 时间基数
		vc->time_base = { 1,outFPS };
		vc->framerate = { outFPS,1 };

		// 画面组大小，多少帧一个关键帧
		vc->gop_size = 50;

		// 
		vc->max_b_frames = 0;

		vc->pix_fmt = AV_PIX_FMT_YUV420P;
		vc->codec_id = AV_CODEC_ID_H264;
		
		av_opt_set(vc->priv_data, "preset", "superfast", 0);
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		//vc->thread_count = 8;

		// 打开编码器
		int ret = avcodec_open2(vc, codec, nullptr);
		if (ret != 0)
		{
			cerr << "avcodec_open2 failed!" << endl;
			return false;
		}
		cout << "avcodec_open2 success!" << endl;

		// 添加视频流到输出上下文
		vs = avformat_new_stream(ic, nullptr);
		vs->codecpar->codec_tag = 0;
		avcodec_parameters_from_context(vs->codecpar, vc);

		av_dump_format(ic, 0, fileName.c_str(), 1);

		// 像素（尺寸）转换上下文,rgb to yuv
		vsc = sws_getCachedContext(
			vsc,
			inWidth, inHeight, (AVPixelFormat)inPixFmt,
			outWidth, outHeight, AV_PIX_FMT_YUV420P,
			SWS_BICUBIC,
			nullptr, nullptr, nullptr
		);
		if (!vsc)
		{
			cerr << "sws_getCachedContext failed!" << endl;
			return false;
		}
		cout << "sws_getCachedContext success!" << endl;

		if (!yuv)
		{
			yuv = av_frame_alloc();
			yuv->format = AV_PIX_FMT_YUV420P;
			yuv->width = outWidth;
			yuv->height = outHeight;
			yuv->pts = 0;
			ret = av_frame_get_buffer(yuv, 32);
			if (ret != 0)
			{
				cerr << "av_frame_get_buffer failed!" << endl;
				return false;
			}
			cout << "av_frame_get_buffer success!" << endl;
		}

		return true;
	}

	bool AddAudioStream() override
	{
		if (!ic)
		{
			return false;
		}

		// 找到音频编码
		AVCodec * codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
		if (!codec)
		{
			cerr << "avcodec_find_encoder failed!" << endl;
			return false;
		}
		cout << "avcodec_find_encoder success!" << endl;

		// 1. 创建并打开音频编码器
		ac = avcodec_alloc_context3(codec);
		if (!ac)
		{
			cerr << "avcodec_alloc_context3 failed!" << endl;
			return false;
		}
		cout << "avcodec_alloc_context3 success!" << endl;

		// 设置参数
		ac->bit_rate = aBitrate;
		ac->sample_rate = outSampleRate;
		ac->sample_fmt = (AVSampleFormat)outSampleFmt;
		ac->channels = outChannels;
		ac->channel_layout = av_get_default_channel_layout(outChannels);
		ac->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		
		int ret = avcodec_open2(ac, codec, nullptr);
		if (ret != 0)
		{
			avcodec_free_context(&ac);
			cerr << "avcodec_open2 failed!" << endl;
			return false;
		}
		cout << "avcodec_open2 AV_CODEC_ID_AAC success!" << endl;

		// 2. 新增音频流
		as = avformat_new_stream(ic, nullptr);
		if (!as)
		{
			cerr << "avformat_new_stream failed!" << endl;
			return false;
		}
		cout << "avformat_new_stream success!" << endl;

		as->codecpar->codec_tag = 0;
		ret = avcodec_parameters_from_context(as->codecpar, ac);
		if (ret < 0)
		{
			cerr << "avcodec_parameters_from_context failed!" << endl;
			return false;
		}
		cout << "avcodec_parameters_from_context success!" << endl;

		av_dump_format(ic, 0, fileName.c_str(), 1);

		// 3. 音频重采样上下文创建
		asc = swr_alloc_set_opts(
			asc,
			ac->channel_layout, ac->sample_fmt, ac->sample_rate,	// 输出格式
			av_get_default_channel_layout(inChannels), (AVSampleFormat)inSampleFmt, inSampleRate,
			0, nullptr
		);
		ret = swr_init(asc);
		if (ret != 0)
		{
			cerr << "swr_init failed!" << endl;
			return false;
		}
		cout << "swr_init success!" << endl;

		// 4. 音频重采样后输出AVFrame
		if (!pcm)
		{
			pcm = av_frame_alloc();
			pcm->format = ac->sample_fmt;
			pcm->channels = ac->channels;
			pcm->channel_layout = ac->channel_layout;
			pcm->nb_samples = nb_sample;	// 一帧音频的样本数量
			ret = av_frame_get_buffer(pcm, 0);
			if (ret < 0)
			{
				cerr << "av_frame_get_buffer failed!" << endl;
				return false;
			}
			cout << "av_frame_get_buffer success!" << endl;
		}

		cout << "audio AVFrame create success!" << endl;
	
		return true;
	}

	AVPacket *EncodeVideo(const unsigned char *rgb) override
	{
		if (!ic || !vsc || !yuv)
		{
			return nullptr;
		}

		AVPacket *pkt = nullptr;
		uint8_t *inData[AV_NUM_DATA_POINTERS] = { nullptr };
		inData[0] = (uint8_t *)rgb;
		int inSize[AV_NUM_DATA_POINTERS] = { 0 };
		inSize[0] = inWidth * 4;
		// rgb to yuv
		int h = sws_scale(vsc, inData, inSize, 0, inHeight,
			yuv->data, yuv->linesize);
		if (h < 0)
		{
			return nullptr;
		}
		//cout << h << "|";
		yuv->pts = vpts;
		vpts++;

		// encode,第二个参数直接传递yuv，会丢失最后几帧数据，一般结尾的几帧数据不太重要，所以大多数就直接丢掉了
		// 如果想处理的话，需要给第二个参数传nullptr，获取缓冲帧数据，以后再研究吧
		int ret = avcodec_send_frame(vc, yuv);
		if (ret != 0)
		{
			return nullptr;
		}
		pkt = av_packet_alloc();
		ret = avcodec_receive_packet(vc, pkt);
		if (ret != 0 || pkt->size <= 0)
		{
			av_packet_free(&pkt);
			return nullptr;
		}

		av_packet_rescale_ts(pkt, vc->time_base, vs->time_base);
		pkt->stream_index = vs->index;

		return pkt;
	}

	AVPacket *EncodeAudio(const unsigned char *d) override
	{
		if (!ic || !asc || !pcm)
		{
			return nullptr;
		}
		// 1 音频重采样
		const uint8_t *data[AV_NUM_DATA_POINTERS] = { nullptr };
		data[0] = (uint8_t *)d;
		int len = swr_convert(asc, pcm->data, pcm->nb_samples,
			data, pcm->nb_samples);
		//cout << len << "*";

		// 2 音频的编码
		int ret = avcodec_send_frame(ac, pcm);
		if (ret != 0)
		{
			return nullptr;
		}

		AVPacket *pkt = av_packet_alloc();
		av_init_packet(pkt);
		ret = avcodec_receive_packet(ac, pkt);
		if (ret != 0)
		{
			av_packet_free(&pkt);
			return nullptr;
		}
		cout << pkt->size << "|";
		pkt->stream_index = as->index;

		pkt->pts = apts;
		pkt->dts = pkt->pts;
		apts += av_rescale_q(pcm->nb_samples, { 1, ac->sample_rate }, ac->time_base);

		return pkt;
	}

	bool WriteHeader() override
	{
		if (!ic)
		{
			return false;
		}

		// 打开io
		int ret = avio_open(&ic->pb, fileName.c_str(), AVIO_FLAG_WRITE);
		if (ret != 0)
		{
			cerr << "avio_open failed!" << endl;
			return false;
		}
		cout << "avio_open success!" << endl;

		// 写入封装头
		ret = avformat_write_header(ic, nullptr);
		if (ret != 0)
		{
			cerr << "avformat_write_header failed!" << endl;
			return false;
		}
		cout << "avformat_write_header success!" << endl;

		return true;
	}

	bool WriteFrame(AVPacket *pkt) override
	{
		if (!ic || !pkt || pkt->size<=0)
		{
			return false;
		}
		if (av_interleaved_write_frame(ic, pkt) != 0)
		{
			return false;
		}

		return true;
	}

	bool WriteEnd() override
	{
		if (!ic || !ic->pb)
		{
			return false;
		}

		// 写入尾部信息索引
		int ret = av_write_trailer(ic);
		if (ret != 0)
		{
			cerr << "av_write_trailer failed!" << endl;
			return false;
		}
		cout << "av_write_trailer success!" << endl;

		// 关闭输入io
		ret = avio_closep(&ic->pb);
		//ret = avio_close(ic->pb);
		if (ret != 0)
		{
			cerr << "avio_close failed!" << endl;
			return false;
		}
		cout << "avio_close success!" << endl;

		cout << "WriteEnd success!" << endl;

		return true;
	}

	bool IsVideoBefore() override
	{
		if (!ic || !asc || !vsc)
		{
			return false;
		}
		int ret = av_compare_ts(vpts, vc->time_base, apts, ac->time_base);
		if (ret <= 0)
		{
			return true;
		}
	
		return false;
	}

public:
	// 封装mp4输出上下文
	AVFormatContext *ic = nullptr;
	// 视频编码器上下文
	AVCodecContext *vc = nullptr;
	// 音频编码器上下文
	AVCodecContext *ac = nullptr;
	// 视频流
	AVStream *vs = nullptr;
	// 音频流
	AVStream *as = nullptr;
	// 像素转换的上下文
	SwsContext *vsc = nullptr;
	// 音频重采样上下文
	SwrContext *asc = nullptr;
	// 输出yuv
	AVFrame *yuv = nullptr;
	// 重采样后输出的pcm
	AVFrame *pcm = nullptr;
	// 视频pts
	int vpts = 0;
	// 音频pts
	int apts = 0;
};


XVideoWriter::XVideoWriter()
{
}


XVideoWriter::~XVideoWriter()
{
}


XVideoWriter *XVideoWriter::Get(unsigned short index)
{
	static  bool isFirst = true;
	if (isFirst)
	{
		av_register_all();
		avcodec_register_all();

		isFirst = false;
	}


	static CXVideoWriter wrs[65535];
	return &wrs[index];
}