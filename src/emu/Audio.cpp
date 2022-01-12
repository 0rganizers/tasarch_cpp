#include "Audio.h"
#include <qmutex.h>

#include <cmath>
#include <iostream>

AudioBuffer::AudioBuffer() {
    open(QIODevice::ReadWrite);
}

qint64 AudioBuffer::readData(char *data, qint64 maxSize) {
    assert(maxSize % 2 == 0); // else not i16, what?!
    QMutexLocker lk(&this->mutex);

    int16_t *samples = (int16_t*) data;
    qint64 numread = 0;
    memset(data, 0, maxSize); // 0 == default

    while(maxSize > numread && !isEmpty()) {
        *samples++ = dequeue();
        numread += 2;
        --numavail;
    }

    return maxSize; // numread?
}

qint64 AudioBuffer::writeData(const char *data, qint64 maxSize) {
    assert(maxSize % 2 == 0); // else not i16, what?!
    QMutexLocker lk(&this->mutex);
    
    int16_t *samples = (int16_t*) data;
    for(int i = 0; i < maxSize / 2; i++) {
        enqueue(samples[i]);
        ++numavail;
    }
    return maxSize;
}

AudioOutput::AudioOutput(double rate) {
    QAudioFormat format;
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setSampleRate(std::floor(rate));
    format.setSampleFormat(QAudioFormat::Int16);

    const auto deviceInfos = QMediaDevices::audioOutputs();
    for (const QAudioDevice &deviceInfo : deviceInfos) {
        if(deviceInfo.isFormatSupported(format)) {
            std::cout << "Device: " << deviceInfo.description().toStdString() << std::endl;
        }
    }

    m_buffer = new AudioBuffer();
    m_audio = new QAudioSink(format);
    m_audio->setVolume(1.00);
    m_audio->start(m_buffer);
}

void AudioOutput::enqueue(int16_t left, int16_t right) {
    m_buffer->write((const char*) &left, 2);
    m_buffer->write((const char*) &right, 2);
}

void AudioOutput::enqueue(const int16_t *data, int64_t len) {
    m_buffer->write((const char*) data, len << 2);
}
