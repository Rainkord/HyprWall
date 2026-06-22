#pragma once
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include "GalleryConstants.h"

class GalleryDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit GalleryDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *p, const QStyleOptionViewItem &opt,
               const QModelIndex &idx) const override
    {
        p->save();
        QRect r = opt.rect;

        int gridW = idx.data(Qt::UserRole + 3).toInt();
        int gridH = idx.data(Qt::UserRole + 4).toInt();
        if (gridW <= 0) gridW = THUMB_W + GRID_PAD;
        if (gridH <= 0) gridH = THUMB_H + LABEL_H;
        int thumbW = gridW - GRID_PAD;
        int thumbH = gridH - LABEL_H;

        bool sel = opt.state & QStyle::State_Selected;
        QColor bg = sel ? QColor(0x38,0x8b,0xfd,50) : QColor(22,27,34,180);
        p->setBrush(bg);
        p->setPen(QPen(sel ? QColor(0x58,0xa6,0xff,180) : QColor(0x30,0x36,0x3d,150), 1));
        QPainterPath path;
        path.addRoundedRect(QRectF(r).adjusted(0.5,0.5,-0.5,-0.5), 5, 5);
        p->setRenderHint(QPainter::Antialiasing);
        p->drawPath(path);

        QRect imgR = r.adjusted(1, 1, -1, -(gridH - thumbH - 1));
        QPixmap px = idx.data(Qt::DecorationRole).value<QIcon>().pixmap(thumbW, thumbH);
        if (!px.isNull()) {
            QPixmap scaled = px.scaled(imgR.size(),
                Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            int cx = (scaled.width()  - imgR.width())  / 2;
            int cy = (scaled.height() - imgR.height()) / 2;
            p->setClipRect(imgR);
            p->drawPixmap(imgR.topLeft(), scaled, QRect(cx, cy, imgR.width(), imgR.height()));
            p->setClipping(false);
        } else {
            p->fillRect(imgR, QColor(30, 35, 42));
            p->setPen(QColor(0x48,0x4f,0x58));
            p->drawText(imgR, Qt::AlignCenter, "...");
        }

        QRect lblR(r.left(), r.bottom() - (gridH - thumbH - 2), r.width(), gridH - thumbH - 1);
        p->setPen(QColor(0x8b,0x94,0x9e));
        QFont f = p->font(); f.setPointSize(8); p->setFont(f);
        QString name = idx.data(Qt::DisplayRole).toString();
        p->drawText(lblR, Qt::AlignCenter | Qt::TextSingleLine,
            p->fontMetrics().elidedText(name, Qt::ElideRight, lblR.width() - 4));

        QRect xR(r.right() - 19, r.top() + 2, 17, 17);
        p->setBrush(QColor(218,54,51,160));
        p->setPen(Qt::NoPen);
        p->drawEllipse(xR);
        p->setPen(QColor(0xff,0xff,0xff,220));
        QFont xf = p->font(); xf.setPointSize(8); xf.setBold(true); p->setFont(xf);
        p->drawText(xR, Qt::AlignCenter, "\u00d7");

        bool locked = idx.data(Qt::UserRole + 2).toBool();
        if (locked) {
            p->setBrush(QColor(13,17,23,110));
            p->setPen(Qt::NoPen);
            p->drawPath(path);
        }

        p->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &idx) const override {
        int gridW = idx.data(Qt::UserRole + 3).toInt();
        int gridH = idx.data(Qt::UserRole + 4).toInt();
        if (gridW <= 0) gridW = THUMB_W + GRID_PAD;
        if (gridH <= 0) gridH = THUMB_H + LABEL_H;
        return QSize(gridW, gridH);
    }
};
