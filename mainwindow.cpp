#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->action_Open_CSV, &QAction::triggered, this, &MainWindow::loadCSV);


    ui->graphingView->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                                    QCP::iSelectLegend | QCP::iSelectPlottables);

    ui->graphingView->xAxis->setRange(0, 8);
    ui->graphingView->yAxis->setRange(0, 255);
    ui->graphingView->axisRect()->setupFullAxesBox();

    ui->graphingView->xAxis->setLabel("Time Axis (uS)");
    ui->graphingView->yAxis->setLabel("Value Axis (V)");
    ui->graphingView->xAxis->setNumberFormat("f");
    ui->graphingView->xAxis->setNumberPrecision(6);
    /*
    ui->graphingView->legend->setVisible(true);
    QFont legendFont = font();
    legendFont.setPointSize(10);
    QFont legendSelectedFont = font();
    legendSelectedFont.setPointSize(12);
    legendSelectedFont.setBold(true);
    ui->graphingView->legend->setFont(legendFont);
    ui->graphingView->legend->setSelectedFont(legendSelectedFont);
    ui->graphingView->legend->setSelectableParts(QCPLegend::spItems); // legend box shall not be selectable, only legend items
    */

    // connect slot that ties some axis selections together (especially opposite axes):
    connect(ui->graphingView, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));
    //connect up the mouse controls
    connect(ui->graphingView, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress()));
    connect(ui->graphingView, SIGNAL(plottableDoubleClick(QCPAbstractPlottable*,int,QMouseEvent*)), this, SLOT(plottableDoubleClick(QCPAbstractPlottable*,int,QMouseEvent*)));
    connect(ui->graphingView, SIGNAL(plottableClick(QCPAbstractPlottable*,int,QMouseEvent*)), this, SLOT(plottableClick(QCPAbstractPlottable*,int,QMouseEvent*)));
    connect(ui->graphingView, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel()));

    // make bottom and left axes transfer their ranges to top and right axes:
    connect(ui->graphingView->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->graphingView->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->graphingView->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->graphingView->yAxis2, SLOT(setRange(QCPRange)));

    //connect(ui->graphingView, SIGNAL(titleDoubleClick(QMouseEvent*,QCPTextElement*)), this, SLOT(titleDoubleClick(QMouseEvent*,QCPTextElement*)));
    connect(ui->graphingView, SIGNAL(axisDoubleClick(QCPAxis*,QCPAxis::SelectablePart,QMouseEvent*)), this, SLOT(axisLabelDoubleClick(QCPAxis*,QCPAxis::SelectablePart)));

    // setup policy and connect slot for context menu popup:
    ui->graphingView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->graphingView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));

    ui->graphingView->setAntialiasedElements(QCP::aeAll);
    //ui->graphingView->setNoAntialiasingOnDrag(true);
    ui->graphingView->setOpenGl(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadCSV()
{
    QString filename;
    QFileDialog dialog;
    bool result = false;

    QStringList filters;
    filters.append(QString(tr("GVRET Logs (*.csv *.CSV)")));

    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilters(filters);
    dialog.setViewMode(QFileDialog::Detail);

    if (dialog.exec() == QDialog::Accepted)
    {
        filename = dialog.selectedFiles()[0];

        QProgressDialog progress(qApp->activeWindow());
        progress.setWindowModality(Qt::WindowModal);
        progress.setLabelText("Loading file...");
        progress.setCancelButton(nullptr);
        progress.setRange(0,0);
        progress.setMinimumDuration(0);
        progress.show();

        qApp->processEvents();

        QFile *inFile = new QFile(filename);
        QByteArray line;
        int lineCounter = 0;
        double timeStamp, dataValue;
        QList<QByteArray> tokens;
        int recordLength = 2000;

        if (!inFile->open(QIODevice::ReadOnly | QIODevice::Text))
        {
            delete inFile;
            return;
        }

        for (int x = 0; x < 16; x++)
        {
            line = inFile->readLine().toUpper(); //read out the header first and discard it.
            tokens = line.split(',');
            if (tokens[0].startsWith("RECORD LENGTH"))
            {
                recordLength = tokens[1].toInt();
            }
        }

        x.reserve(recordLength);
        y.reserve(recordLength);

        double yminval=10000000.0, ymaxval = -1000000.0;
        double xminval=10000000000.0, xmaxval = -10000000000.0;

        while (!inFile->atEnd()) {
            lineCounter++;
            if (lineCounter > 100)
            {
                qApp->processEvents();
                lineCounter = 0;
            }

            line = inFile->readLine().simplified();
            if (line.length() > 2)
            {
                tokens = line.split(',');
                if (tokens.length() >= 2)
                {
                    timeStamp = tokens[0].toDouble() * 1000000.0; //multiply by a million
                    dataValue = tokens[1].toDouble();
                    x.append(timeStamp);
                    y.append(dataValue);
                    if (dataValue < yminval) yminval = dataValue;
                    if (dataValue > ymaxval) ymaxval = dataValue;
                    if (timeStamp < xminval) xminval = timeStamp;
                    if (timeStamp > xmaxval) xmaxval = timeStamp;
                }
            }
        }
        inFile->close();
        delete inFile;

        ui->graphingView->addGraph();

        ui->graphingView->graph()->setData(x,y);
        ui->graphingView->graph()->setLineStyle(QCPGraph::lsLine); //connect points with lines
        QPen graphPen;
        graphPen.setColor(QColor::fromRgbF(0.0, 0.0, 1.0));
        graphPen.setWidth(1);
        ui->graphingView->graph()->setPen(graphPen);

        qDebug() << "xmin: " << xminval;
        qDebug() << "xmax: " << xmaxval;
        qDebug() << "ymin: " << yminval;
        qDebug() << "ymax: " << ymaxval;

        ui->graphingView->xAxis->setRange(xminval, xmaxval);
        ui->graphingView->yAxis->setRange(yminval, ymaxval);
        ui->graphingView->axisRect()->setupFullAxesBox();

        ui->graphingView->replot();

        progress.cancel();
    }
}

void MainWindow::plottableClick(QCPAbstractPlottable* plottable, int dataIdx, QMouseEvent* event)
{
    Q_UNUSED(dataIdx);
    qDebug() << "plottableClick";
    double x, y;
    QCPGraph *graph = reinterpret_cast<QCPGraph *>(plottable);
    graph->pixelsToCoords(event->localPos(), x, y);
}

void MainWindow::plottableDoubleClick(QCPAbstractPlottable* plottable, int dataIdx, QMouseEvent* event)
{
    Q_UNUSED(dataIdx);
    qDebug() << "plottableDoubleClick";
    int id = 0;
    //apply transforms to get the X axis value where we double clicked
    double coord = plottable->keyAxis()->pixelToCoord(event->localPos().x());
    id = plottable->property("id").toInt();

    double x, y;
    QCPGraph *graph = reinterpret_cast<QCPGraph *>(plottable);
    graph->pixelsToCoords(event->localPos(), x, y);
    x = ui->graphingView->xAxis->pixelToCoord(event->localPos().x());
}

void MainWindow::titleDoubleClick(QMouseEvent* event, QCPTextElement* title)
{
  Q_UNUSED(event)
  Q_UNUSED(title)
    qDebug() << "title Double Click";
  // Set the plot title by double clicking on it

    /*
  bool ok;
  QString newTitle = QInputDialog::getText(this, "SavvyCAN Graphing", "New plot title:", QLineEdit::Normal, title->text(), &ok);
  if (ok)
  {
    title->setText(newTitle);
    ui->graphingView->replot();
  } */

}

void MainWindow::axisLabelDoubleClick(QCPAxis *axis, QCPAxis::SelectablePart part)
{
  qDebug() << "axisLabelDoubleClick";
  // Set an axis label by double clicking on it
  if (part == QCPAxis::spAxisLabel) // only react when the actual axis label is clicked, not tick label or axis backbone
  {
    bool ok;
    QString newLabel = QInputDialog::getText(this, "SavvyCAN Graphing", "New axis label:", QLineEdit::Normal, axis->label(), &ok);
    if (ok)
    {
      axis->setLabel(newLabel);
      ui->graphingView->replot();
    }
  }
}

void MainWindow::selectionChanged()
{
  /*
   normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
   the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
   and the axis base line together. However, the axis label shall be selectable individually.

   The selection state of the left and right axes shall be synchronized as well as the state of the
   bottom and top axes.

   Further, we want to synchronize the selection of the graphs with the selection state of the respective
   legend item belonging to that graph. So the user can select a graph by either clicking on the graph itself
   or on its legend item.
  */

    qDebug() << "SelectionChanged";

  // make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
  if (ui->graphingView->xAxis->selectedParts().testFlag(QCPAxis::spAxis) || ui->graphingView->xAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
      ui->graphingView->xAxis2->selectedParts().testFlag(QCPAxis::spAxis) || ui->graphingView->xAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    ui->graphingView->xAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    ui->graphingView->xAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }
  // make left and right axes be selected synchronously, and handle axis and tick labels as one selectable object:
  if (ui->graphingView->yAxis->selectedParts().testFlag(QCPAxis::spAxis) || ui->graphingView->yAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
      ui->graphingView->yAxis2->selectedParts().testFlag(QCPAxis::spAxis) || ui->graphingView->yAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    ui->graphingView->yAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    ui->graphingView->yAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }

  // synchronize selection of graphs with selection of corresponding legend items:
  for (int i=0; i<ui->graphingView->graphCount(); ++i)
  {
    QCPGraph *graph = ui->graphingView->graph(i);
    QCPPlottableLegendItem *item = ui->graphingView->legend->itemWithPlottable(graph);
    if (item->selected() || graph->selected())
    {
      item->setSelected(true);
      //select graph too.
      QCPDataSelection sel;
      QCPDataRange rang;
      rang.setBegin(0);
      rang.setEnd(graph->dataCount());
      sel.addDataRange(rang);
      graph->setSelection(sel);
    }
  }
}

void MainWindow::mousePress()
{
  // if an axis is selected, only allow the direction of that axis to be dragged
  // if no axis is selected, both directions may be dragged

  if (ui->graphingView->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    ui->graphingView->axisRect()->setRangeDrag(ui->graphingView->xAxis->orientation());
  else if (ui->graphingView->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    ui->graphingView->axisRect()->setRangeDrag(ui->graphingView->yAxis->orientation());
  else
    ui->graphingView->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
}

void MainWindow::mouseWheel()
{
  // if an axis is selected, only allow the direction of that axis to be zoomed
  // if no axis is selected, both directions may be zoomed

  if (ui->graphingView->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    ui->graphingView->axisRect()->setRangeZoom(ui->graphingView->xAxis->orientation());
  else if (ui->graphingView->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    ui->graphingView->axisRect()->setRangeZoom(ui->graphingView->yAxis->orientation());
  else
    ui->graphingView->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
}

/*
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key())
        {
        case Qt::Key_Plus:
            zoomIn();
            break;
        case Qt::Key_Minus:
            zoomOut();
            break;
        }
        return true;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }

    return false;
}
*/

void MainWindow::resetView()
{
    double yminval=10000000.0, ymaxval = -1000000.0;
    double xminval=100000000000, xmaxval = -10000000000.0;

    ui->graphingView->xAxis->setRange(xminval, xmaxval);
    ui->graphingView->yAxis->setRange(yminval, ymaxval);
    ui->graphingView->axisRect()->setupFullAxesBox();

    ui->graphingView->replot();
}

void MainWindow::zoomIn()
{
    QCPRange xrange = ui->graphingView->xAxis->range();
    QCPRange yrange = ui->graphingView->yAxis->range();
    if (ui->graphingView->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    {
        ui->graphingView->xAxis->scaleRange(0.666, xrange.center());
    }

    else if (ui->graphingView->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    {
        ui->graphingView->yAxis->scaleRange(0.666, yrange.center());
    }
    else
    {
        ui->graphingView->xAxis->scaleRange(0.666, xrange.center());
        ui->graphingView->yAxis->scaleRange(0.666, yrange.center());
    }
    ui->graphingView->replot();
}

void MainWindow::zoomOut()
{
    QCPRange xrange = ui->graphingView->xAxis->range();
    QCPRange yrange = ui->graphingView->yAxis->range();
    if (ui->graphingView->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    {
        ui->graphingView->xAxis->scaleRange(1.5, xrange.center());
    }

    else if (ui->graphingView->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    {
        ui->graphingView->yAxis->scaleRange(1.5, yrange.center());
    }
    else
    {
        ui->graphingView->xAxis->scaleRange(1.5, xrange.center());
        ui->graphingView->yAxis->scaleRange(1.5, yrange.center());
    }
    ui->graphingView->replot();
}

void MainWindow::contextMenuRequest(QPoint pos)
{
  QMenu *menu = new QMenu(this);
  menu->setAttribute(Qt::WA_DeleteOnClose);

  {
    menu->addAction(tr("Save graph image to file"), this, SLOT(saveGraphs()));
    menu->addAction(tr("Save graph definitions to file"), this, SLOT(saveDefinitions()));
    menu->addSeparator();
    menu->addAction(tr("Reset View"), this, SLOT(resetView()));
    menu->addAction(tr("Zoom In"), this, SLOT(zoomIn()));
    menu->addAction(tr("Zoom Out"), this, SLOT(zoomOut()));
  }

  menu->popup(ui->graphingView->mapToGlobal(pos));
}
