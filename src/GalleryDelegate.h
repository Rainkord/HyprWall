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

    void setAnchorRow(int row) { m_anchorRow = row; }
    int anchorRow() const { return m_anchorRow; }

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

        bool hovered = opt.state & QStyle::State_MouseOver;
        bool anchor = (idx.row() == m_anchorRow);

        // Card background
        QColor bg = anchor ? QColor(0x38,0x8b,0xfd,100)
                           : hovered ? QColor(0x30,0x36,0x3d,120)
                                     : QColor(22,27,34,180);
        p->setBrush(bg);
        p->setPen(QPen(anchor ? QColor(0x58,0xa6,0xff,255)
                              : hovered ? QColor(0x48,0x4f,0x58,150)
                                        : QColor(0x30,0x36,0x3d,100), anchor ? 2 : 1));
        QPainterPath path;
        path.addRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);
        p->setRenderHint(QPainter::Antialiasing);
        p->drawPath(path);

        // Hover glow — subtle
        if (hovered && !anchor) {
            QRadialGradient glow(QPointF(r.center().x(), r.top()), r.width() * 0.6);
            glow.setColorAt(0.0, QColor(0x58,0xa6,0xff, 25));
            glow.setColorAt(0.5, QColor(0x58,0xa6,0xff, 8));
            glow.setColorAt(1.0, QColor(0x58,0xa6,0xff, 0));
            p->setPen(Qt::NoPen);
            p->setBrush(glow);
            p->drawRoundedRect(r.adjusted(-2,-2,2,2), 9, 9);
        }

        // Image area
        QRect imgR = r.adjusted(1, 1, -1, -(gridH - thumbH - 1));
        QIcon icon = idx.data(Qt::DecorationRole).value<QIcon>();
        if (!icon.isNull()) {
            QPixmap px = icon.pixmap(imgR.size());
            if (!px.isNull()) {
                QPixmap scaled = px.scaled(imgR.size(),
                    Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                int cx = (scaled.width()  - imgR.width())  / 2;
                int cy = (scaled.height() - imgR.height()) / 2;

                QPainterPath imgPath;
                imgPath.addRoundedRect(QRectF(imgR), 7, 7);
                p->setClipPath(imgPath);
                p->drawPixmap(imgR.topLeft(), scaled, QRect(cx, cy, imgR.width(), imgR.height()));
                p->setClipping(false);
            } else {
                p->fillRect(imgR, QColor(30, 35, 42));
                p->setPen(QColor(0x48,0x4f,0x58));
                p->drawText(imgR, Qt::AlignCenter, "...");
            }
        } else {
            p->fillRect(imgR, QColor(30, 35, 42));
            p->setPen(QColor(0x48,0x4f,0x58));
            p->drawText(imgR, Qt::AlignCenter, "...");
        }

        // Filename label
        QRect lblR(r.left() + 2, r.bottom() - (gridH - thumbH - 1),
                   r.width() - 4, gridH - thumbH - 2);
        p->setPen(anchor ? QColor(0xe6,0xed,0xf3) : hovered ? QColor(0xe6,0xed,0xf3) : QColor(0x8b,0x94,0x9e));
        QFont f = p->font(); f.setPointSize(8); p->setFont(f);
        QString name = idx.data(Qt::DisplayRole).toString();
        p->drawText(lblR, Qt::AlignCenter | Qt::TextSingleLine,
            p->fontMetrics().elidedText(name, Qt::ElideRight, lblR.width() - 4));

        // Delete button — only on hover
        if (hovered || anchor) {
            QRect xR(r.right() - 20, r.top() + 3, 18, 18);
            QColor xBg = hovered ? QColor(248, 81, 73, 200) : QColor(248, 81, 73, 140);
            p->setBrush(xBg);
            p->setPen(Qt::NoPen);
            p->drawEllipse(xR);
            p->setPen(QColor(0xff, 0xff, 0xff, 230));
            QFont xf = p->font(); xf.setPointSize(9); xf.setBold(true); p->setFont(xf);
            p->drawText(xR, Qt::AlignCenter, "\u00d7");
        }

        // Locked overlay
        bool locked = idx.data(Qt::UserRole + 2).toBool();
        if (locked) {
            p->setBrush(QColor(13, 17, 23, 110));
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

private:
    int m_anchorRow = -1;
};
