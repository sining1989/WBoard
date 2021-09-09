#include "WBAbstractVideoEncoder.h"

#include "core/memcheck.h"

WBAbstractVideoEncoder::WBAbstractVideoEncoder(QObject *pParent)
    : QObject(pParent)
    , mFramesPerSecond(10)
    , mVideoSize(640, 480)
    , mVideoBitsPerSecond(1700000) // 1.7 Mbps
{
    // NOOP

}

WBAbstractVideoEncoder::~WBAbstractVideoEncoder()
{
    // NOOP
}


void WBAbstractVideoEncoder::newChapter(const QString& pLabel, long timestamp)
{
    Q_UNUSED(pLabel);
    Q_UNUSED(timestamp);
}
