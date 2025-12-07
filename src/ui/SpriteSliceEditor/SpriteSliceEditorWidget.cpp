#include "SpriteSliceEditorWidget.h"
#include "ui_SpriteSliceEditorWidget.h"
#include "ui/Common.h"

#include <QMouseEvent>

SpriteSliceEditorWidget::SpriteSliceEditorWidget(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::SpriteSliceEditorWidget)
{
	setAttribute(Qt::WA_StyledBackground, true);
	ui->setupUi(this);
	LOAD_QSS(":/SpriteSliceEditor/SpriteSliceEditor.qss");

	initSliceTable();
	
	qDebug() << "The Object Name: [ " << objectName() << " ]\n";

	// 示例：设置 slice 表格列宽自适应
	ui->sliceTableWidget->horizontalHeader()->setStretchLastSection(true);
	ui->sliceTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	// 示例：预填一些 Group 下拉框选项
	ui->groupComboBox->addItem(QStringLiteral("Tiles"));
	ui->groupComboBox->addItem(QStringLiteral("Props"));
	ui->groupComboBox->addItem(QStringLiteral("Characters"));
	ui->groupComboBox->addItem(QStringLiteral("UI"));

	// 默认选中 Grid 模式、显示网格
	ui->actionGridMode->setChecked(true);
	ui->actionToggleGrid->setChecked(true);

	connect(ui->btnReturn, &QPushButton::clicked, this, &SpriteSliceEditorWidget::SignalReturnToMainPanel);

	QWidget* header = this->findChild<QWidget*>("headerBarWidget");
	if (header)
	{
		header->setMouseTracking(true);
		header->installEventFilter(this);
		header->setCursor(Qt::ArrowCursor);
	}
}

SpriteSliceEditorWidget::~SpriteSliceEditorWidget()
{
	delete ui;
}

bool SpriteSliceEditorWidget::eventFilter(QObject* watched, QEvent* event)
{
	QWidget* header = this->findChild<QWidget*>("headerBarWidget");
	if (watched == header)
	{
		switch (event->type())
		{
		case QEvent::MouseButtonPress:
		{
			auto* me = reinterpret_cast<QMouseEvent*>(event);
			if (me->button() == Qt::LeftButton)
			{
				m_dragging = true;
				m_dragStartGlobal = me->globalPosition().toPoint();
				m_windowStartTopLeft = window()->frameGeometry().topLeft();
				return true;
			}
			break;
		}
		case QEvent::MouseMove:
		{
			if (m_dragging)
			{
				auto* me = static_cast<QMouseEvent*>(event);
				const QPoint currentGlobal = me->globalPosition().toPoint();
				const QPoint delta = currentGlobal - m_dragStartGlobal;

				window()->move(m_windowStartTopLeft + delta);
				return true;
			}
			break;
		}
		case QEvent::MouseButtonRelease:
		{
			auto* me = static_cast<QMouseEvent*>(event);
			if (me->button() == Qt::LeftButton)
			{
				m_dragging = false;
				return true;
			}
			break;
		}
		default:
			break;
		}
	}
	return QWidget::eventFilter(watched, event);
}

void SpriteSliceEditorWidget::initSliceTable()
{
	auto* table = findChild<QTableWidget*>("sliceTableWidget");
	if (!table) return;

	// 表头行为与分隔线
	auto* hv = table->horizontalHeader();
	hv->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	hv->setStretchLastSection(true);          // 最后一列填充
	hv->setMinimumSectionSize(80);
	hv->setHighlightSections(false);

	// 设置列宽（可按需调整）
	const int colPreview = 0;
	const int colName = 1;
	table->setColumnWidth(colPreview, 80);
	table->setColumnWidth(colName, 160);

	// 插入两条假数据
	table->setRowCount(2);

	// 行0
	{
		// 预览：使用一个占位缩略
		QImage thumb(64, 64, QImage::Format_ARGB32_Premultiplied);
		thumb.fill(QColor(240, 243, 248));
		QPainter p(&thumb);
		p.setRenderHint(QPainter::Antialiasing);
		p.setPen(QPen(QColor(148, 181, 217), 2));
		p.drawRect(6, 6, 64 - 12, 64 - 12);
		p.end();

		auto* previewLabel = new QLabel;
		previewLabel->setPixmap(QPixmap::fromImage(thumb));
		previewLabel->setAlignment(Qt::AlignCenter);
		table->setCellWidget(0, colPreview, previewLabel);

		table->setItem(0, 1, new QTableWidgetItem("grass_0_0"));
		table->setItem(0, 2, new QTableWidgetItem(QString::number(0)));   // X
		table->setItem(0, 3, new QTableWidgetItem(QString::number(0)));   // Y
		table->setItem(0, 4, new QTableWidgetItem(QString::number(32)));  // W
		table->setItem(0, 5, new QTableWidgetItem(QString::number(32)));  // H
		table->setItem(0, 6, new QTableWidgetItem("tileset_terrain"));    // Group
		table->setItem(0, 7, new QTableWidgetItem("terrain,walkable"));   // Tags
	}

	// 行1
	{
		QImage thumb(64, 64, QImage::Format_ARGB32_Premultiplied);
		thumb.fill(QColor(240, 243, 248));
		QPainter p(&thumb);
		p.setRenderHint(QPainter::Antialiasing);
		p.setPen(QPen(QColor(176, 56, 56), 2));
		p.drawEllipse(QRect(10, 10, 44, 44));
		p.end();

		auto* previewLabel = new QLabel;
		previewLabel->setPixmap(QPixmap::fromImage(thumb));
		previewLabel->setAlignment(Qt::AlignCenter);
		table->setCellWidget(1, colPreview, previewLabel);

		table->setItem(1, 1, new QTableWidgetItem("rock_0_1"));
		table->setItem(1, 2, new QTableWidgetItem(QString::number(32)));  // X
		table->setItem(1, 3, new QTableWidgetItem(QString::number(0)));   // Y
		table->setItem(1, 4, new QTableWidgetItem(QString::number(32)));  // W
		table->setItem(1, 5, new QTableWidgetItem(QString::number(32)));  // H
		table->setItem(1, 6, new QTableWidgetItem("tileset_terrain"));    // Group
		table->setItem(1, 7, new QTableWidgetItem("terrain,blocking"));   // Tags
	}

	// 对齐与不可编辑
	for (int r = 0; r < table->rowCount(); ++r) {
		for (int c = 1; c < table->columnCount(); ++c) {
			if (auto* item = table->item(r, c)) {
				if (c >= 2 && c <= 5) item->setTextAlignment(Qt::AlignCenter);
				item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			}
		}
	}
}

