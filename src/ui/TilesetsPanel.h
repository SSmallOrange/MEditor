#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QToolButton>
#include "ui/Common.h"
#include "ui_TilesetsPanel.h"
#include "TilesetBlockWidget.h"

class TilesetsPanel : public QWidget
{
	Q_OBJECT
public:
	explicit TilesetsPanel(QWidget* parent = nullptr)
		: QWidget(parent)
	{
		setObjectName("TilesetsPanel");
		setAttribute(Qt::WA_StyledBackground, true);
		ui.setupUi(this);
		QFile f(":/TilesetsPanel/TilesetsPanel.qss");
		qDebug() << "Resource exists?" << f.exists();
		if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
			const QString css = QString::fromUtf8(f.readAll());
			qDebug() << "Loaded QSS length =" << css.size();
			qDebug().noquote() << css.left(120);
			setStyleSheet(css);
		}
		else {
			qWarning() << "Open failed";
		}
		qDebug() << "Panel objectName =" << objectName();
		connectSignals();
	}

	Ui::TilesetsPanel ui;

signals:
	void searchTextChanged(const QString& text);
	void addTilesetRequested();
	void tileSelected(const QString& tilesetId, int tileIndex);
	void tilesetRemoved(const QString& tilesetId);
	void tilesetCollapsedChanged(const QString& tilesetId, bool collapsed);

private slots:
	void onAddTileset();

private:
	void connectSignals();
	void insertTilesetWidget(TilesetBlockWidget* w);
	void removeTilesetById(const QString& id);
};