#include <QAudioSink>
#include <QAudioDevice>
#include <QBuffer>
#include <QQueue>
#include <QMediaDevices>

// ...
class AudioBuffer : public QIODevice, public QQueue<int16_t> {
    public:
        AudioBuffer();
        qint64 numavail = 0;

    protected:
        qint64 writeData(const char *data, qint64 maxSize) override;
        qint64 readData(char *data, qint64 maxSize) override;
        inline bool isSequential() const override { return true; }
        inline qint64 bytesAvailable() const override { return QIODevice::bytesAvailable() + (16);}
};

class AudioOutput {
    protected:
        AudioBuffer* m_buffer;
        QAudioSink *m_audio;

    public:
        AudioOutput(double rate);

        void enqueue(int16_t left, int16_t right);
        void enqueue(const int16_t *stream, int64_t len);
};
