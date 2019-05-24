#include "internal/SampleBufferFileHelper.h"

#include <QFileInfo>


#include <sndfile.h>
#include <QtCore/QDir>
#include <QtWidgets/QMessageBox>
#include "DrumSynth.h"
#include "FileDialog.h"


#ifdef LMMS_HAVE_OGGVORBIS
#define OV_EXCLUDE_STATIC_CALLBACKS

#include <vorbis/vorbisfile.h>

#endif

#include "internal/SampleBufferFileHelper.h"
#include "GuiApplication.h"
#include "ConfigManager.h"
#include "Mixer.h"
#include "endian_handling.h"


namespace internal {
	// TODO: move these to interfaces.
	// (for another commit)
	typedef SampleBufferData::DataVector (FileDecoderFunc)(const QString &fileName,
														   ch_cnt_t &_channels, sample_rate_t &_samplerate,
														   QString &loadingWarning, bool &isError);

	namespace FileDecoders {
		FileDecoderFunc decodeSampleSF;
		FileDecoderFunc decodeSampleOGGVorbis;
		FileDecoderFunc decodeSampleDS;
	}


	constexpr FileDecoderFunc *fileDecodingFunctions[] = {
			FileDecoders::decodeSampleSF,
			FileDecoders::decodeSampleOGGVorbis,
			FileDecoders::decodeSampleDS
	};

	SampleBufferData::DataVector convertIntToFloat(int_sample_t *&_ibuf, f_cnt_t _frames, int _channels);
}

internal::SampleBufferData::DataVector internal::FileDecoders::decodeSampleSF(const QString &_f,
																			  ch_cnt_t &_channels,
																			  sample_rate_t &_samplerate,
																			  QString &loadingWarning,
																			  bool &isError) {
	SNDFILE *snd_file;
	SF_INFO sf_info;
	sf_info.format = 0;
	f_cnt_t frames = 0;
	SampleBufferData::DataVector vector;
	bool sf_rr = false;


	// Use QFile to handle unicode file names on Windows
	QFile f(_f);
	f.open(QIODevice::ReadOnly);
	if ((snd_file = sf_open_fd(f.handle(), SFM_READ, &sf_info, false)) != nullptr) {
		frames = sf_info.frames;
		const auto channels = sf_info.channels;

		// TODO: remove that.
		MmAllocator<float>::vector buffer(channels * frames);
		sf_rr = sf_read_float(snd_file, buffer.data(), channels * frames);

		if (sf_info.channels > DEFAULT_CHANNELS) {
			loadingWarning = QObject::tr("The file you've selected has %1 channels. LMMS support "
										 "Stereo and Mono.").arg(sf_info.channels);
		}

		// Copy buffer using stereo
		vector.resize(frames);
		auto rightOffset = sf_info.channels > 1 ? 1 : 0;
		for (size_t i = 0; i < frames; i++) {
			vector[i][0] = buffer[i * channels];
			vector[i][1] = buffer[i * channels + rightOffset];
		}

		if (sf_rr < sf_info.channels * frames) {
#ifdef DEBUG_LMMS
			qDebug( "SampleBuffer::decodeSampleSF(): could not read"
				" sample %s: %s", _f, sf_strerror( NULL ) );
#endif
		}
		_channels = sf_info.channels;
		_samplerate = sf_info.samplerate;

		sf_close(snd_file);
	} else {
#ifdef DEBUG_LMMS
		qDebug( "SampleBuffer::decodeSampleSF(): could not load "
				"sample %s: %s", _f, sf_strerror( NULL ) );
#endif
		loadingWarning = QObject::tr("SoundFile: Could not load: %1").arg(sf_strerror(nullptr));
		isError = true;
	}
	f.close();

	return vector;
}


#ifdef LMMS_HAVE_OGGVORBIS

// callback-functions for reading ogg-file

size_t qfileReadCallback(void *_ptr, size_t _size, size_t _n, void *_udata) {
	return static_cast<size_t>(static_cast<QFile *>( _udata )->read((char *) _ptr,
																	_size * _n));
}


int qfileSeekCallback(void *_udata, ogg_int64_t _offset, int _whence) {
	auto *f = static_cast<QFile *>( _udata );

	if (_whence == SEEK_CUR) {
		f->seek(f->pos() + _offset);
	} else if (_whence == SEEK_END) {
		f->seek(f->size() + _offset);
	} else {
		f->seek(_offset);
	}
	return 0;
}


int qfileCloseCallback(void *_udata) {
	delete static_cast<QFile *>( _udata );
	return 0;
}


long qfileTellCallback(void *_udata) {
	return static_cast<QFile *>( _udata )->pos();
}


internal::SampleBufferData::DataVector internal::FileDecoders::decodeSampleOGGVorbis(const QString &_f,
																					 ch_cnt_t &_channels,
																					 sample_rate_t &_samplerate,
																					 QString &loadingWarning,
																					 bool &isError) {
	static ov_callbacks callbacks =
			{
					qfileReadCallback,
					qfileSeekCallback,
					qfileCloseCallback,
					qfileTellCallback
			};

	OggVorbis_File vf;

	f_cnt_t frames = 0;

	auto *f = new QFile(_f);
	if (!f->open(QFile::ReadOnly)) {
		delete f;
		isError = true;
		return {};
	}

	int err = ov_open_callbacks(f, &vf, nullptr, 0, callbacks);

	if (err < 0) {
		switch (err) {
			case OV_EREAD:
				printf("SampleBuffer::decodeSampleOGGVorbis():"
					   " media read error\n");
				break;
			case OV_ENOTVORBIS:
/*				printf( "SampleBuffer::decodeSampleOGGVorbis():"
					" not an Ogg Vorbis file\n" );*/
				break;
			case OV_EVERSION:
				printf("SampleBuffer::decodeSampleOGGVorbis():"
					   " vorbis version mismatch\n");
				break;
			case OV_EBADHEADER:
				printf("SampleBuffer::decodeSampleOGGVorbis():"
					   " invalid Vorbis bitstream header\n");
				break;
			case OV_EFAULT:
				printf("SampleBuffer::decodeSampleOgg(): "
					   "internal logic fault\n");
				break;
		}
		delete f;

		isError = true;
		return {};
	}

	ov_pcm_seek(&vf, 0);

	_channels = ov_info(&vf, -1)->channels;
	_samplerate = ov_info(&vf, -1)->rate;

	ogg_int64_t total = ov_pcm_total(&vf, -1);

	auto _buf = new int_sample_t[total * _channels];
	int bitstream = 0;
	long bytes_read = 0;

	do {
		bytes_read = ov_read(&vf, (char *) &_buf[frames * _channels],
							 (total - frames) * _channels *
							 BYTES_PER_INT_SAMPLE,
							 isLittleEndian() ? 0 : 1,
							 BYTES_PER_INT_SAMPLE, 1, &bitstream);
		if (bytes_read < 0) {
			break;
		}
		frames += bytes_read / (_channels * BYTES_PER_INT_SAMPLE);
	} while (bytes_read != 0 && bitstream == 0);

	ov_clear(&vf);
	// if buffer isn't empty, convert it to float and write it down

	if (frames > 0) {
		return convertIntToFloat(_buf, frames, _channels);
	} else if (frames < 0) {
		isError = true;
	}

	return {};
}

#endif

internal::SampleBufferData::DataVector
internal::FileDecoders::decodeSampleDS(const QString &_f, ch_cnt_t &_channels,
									   sample_rate_t &_samplerate,
									   QString &loadingWarning,
									   bool &isError) {
	DrumSynth ds;
	int_sample_t *_buf = nullptr;
	f_cnt_t frames = ds.GetDSFileSamples(_f, _buf, _channels, _samplerate);

	if (frames > 0 && _buf != nullptr) {
		return convertIntToFloat(_buf, frames, _channels);
	}

	isError = true;
	return {};
}

internal::SampleBufferData
internal::SampleBufferFileHelper::Load(internal::SampleBufferFileHelper::FileName fileName, bool ignoreError) {
	fileName = tryToMakeRelative(fileName);

	sample_rate_t sampleRate = mixerSampleRate();
	if (fileName == "")
		return {{}, sampleRate};

	bool fileLoadError = false;

	ch_cnt_t channels = DEFAULT_CHANNELS;

	const QFileInfo fileInfo(fileName);

	SampleBufferData::DataVector fileData;
	QString loadingWarning;
	if (fileInfo.suffix() == "ogg") {
		fileLoadError = false;
		fileData = FileDecoders::decodeSampleOGGVorbis(fileName, channels, sampleRate, loadingWarning,
													   fileLoadError);
	}
	for (auto function : fileDecodingFunctions) {
		fileData = function(fileName, channels, sampleRate, loadingWarning, fileLoadError);
		if (!fileLoadError)
			break;
	}

	if (fileLoadError) {
		if (!ignoreError) {
			QString title = QObject::tr("Fail to open file");
			QString message = QObject::tr("No message");
			if (!loadingWarning.isEmpty())
				message = loadingWarning;
			if (gui) {
				QMessageBox::information(nullptr,
										 title, message, QMessageBox::Warning);
			} else {
				fprintf(stderr, "%s\n", message.toUtf8().constData());
			}
		}

		return {{}, mixerSampleRate()};
	} else {
		return {std::move(fileData), sampleRate};
	}
}

QString internal::SampleBufferFileHelper::tryToMakeRelative(const QString &file) {
	if (!QFileInfo(file).isRelative()) {
		// Normalize the path
		QString f(QDir::cleanPath(file));

		// First, look in factory samples
		// Isolate "samples/" from "data:/samples/"
		QString samplesSuffix = ConfigManager::inst()->factorySamplesDir().mid(
				ConfigManager::inst()->dataDir().length());

		// Iterate over all valid "data:/" searchPaths
		for (const QString &path : QDir::searchPaths("data")) {
			QString samplesPath = QDir::cleanPath(path + samplesSuffix) + "/";
			if (f.startsWith(samplesPath)) {
				return QString(f).mid(samplesPath.length());
			}
		}

		// Next, look in user samples
		QString usd = ConfigManager::inst()->userSamplesDir();
		usd.replace(QDir::separator(), '/');
		if (f.startsWith(usd)) {
			return QString(f).mid(usd.length());
		}
	}
	return file;
}

internal::SampleBufferData::DataVector
internal::convertIntToFloat(int_sample_t *&_ibuf, f_cnt_t _frames, int _channels) {
	// following code transforms int-samples into
	// float-samples and does amplifying & reversing
	const float fac = 1 / OUTPUT_SAMPLE_MULTIPLIER;
	SampleBufferData::DataVector vector(_frames);
	const int ch = (_channels > 1) ? 1 : 0;

	int idx = 0;
	for (f_cnt_t frame = 0; frame < _frames;
		 ++frame) {
		vector[frame][0] = _ibuf[idx + 0] * fac;
		vector[frame][1] = _ibuf[idx + ch] * fac;
		idx += _channels;
	}

	delete[] _ibuf;

	return vector;
}
