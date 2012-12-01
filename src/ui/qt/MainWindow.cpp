#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QTextCursor>
#include <QTextTableFormat>
#include <QDebug>
#include <QByteArray>
#include <QMessageBox>
#include <QKeySequence>
#include <QScrollBar>

#include "MainWindow.hpp"
#include "Settings.hpp"
#include "Proxy.hpp"

#include "medusa/log.hpp"

MEDUSA_NAMESPACE_USE

MainWindow::MainWindow()
  : QMainWindow(), Ui::MainWindow()
  , _about(this)
  , _openConfirmation(this)
  , _loaderChooser(this, this->_medusa)
  , _goto(this)
  , _settingsDialog(this)
  , _undoJumpView()
  , _documentOpened(false)
  , _fileName()
  , _closeWindow(false)
  , _openDocument(false)
  , _medusa()
  , _selectedLoader()
  , _disasmView(this)
{
  this->setupUi(this);

  medusa::Log::SetLog(boost::bind(&MainWindow::appendLog, this, _1));

  setCentralWidget(&_disasmView);

  connect(this->dataList,     SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(_on_label_clicked(QListWidgetItem *)));
  connect(this->codeList,     SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(_on_label_clicked(QListWidgetItem *)));
  connect(this->stringList,   SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(_on_label_clicked(QListWidgetItem *)));
  connect(this->importedList, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(_on_label_clicked(QListWidgetItem *)));
  connect(this->exportedList, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(_on_label_clicked(QListWidgetItem *)));

  connect(this, SIGNAL(logAppended(QString const &)), this, SLOT(onLogMessageAppended(QString const &)));

  connect(this, SIGNAL(disassemblyListingUpdated()), &_disasmView, SLOT(listingUpdated()));
  connect(&Settings::instance(), SIGNAL(settingsChanged()), &_disasmView, SLOT(setFont()));

  qRegisterMetaType<medusa::Label>("medusa::Label");
  connect(this, SIGNAL(labelAdded(medusa::Label const&)), this, SLOT(onLabelAdded(medusa::Label const&)));

  this->restoreGeometry(Settings::instance().value(WINDOW_GEOMETRY, WINDOW_GEOMETRY_DEFAULT).toByteArray());
  this->restoreState(Settings::instance().value(WINDOW_LAYOUT, WINDOW_LAYOUT_DEFAULT).toByteArray());
}

MainWindow::~MainWindow()
{
}

bool    MainWindow::openDocument()
{
  _fileName = QFileDialog::getOpenFileName(this, tr("Select a file"));

  if (_fileName.isNull())
    return (false);

  // Opening file and loading module
  _medusa.Open(this->_fileName.toStdWString());
  _medusa.LoadModules(L".");
  _medusa.GetDatabase().StartsEventHandling(new EventProxy(this));

  emit logAppended(QString("Opening %1\n").arg(this->_fileName));

  medusa::Loader::VectorSharedPtr const & loaders = this->_medusa.GetSupportedLoaders();

  // If no compatible loader was found
  if (loaders.empty())
  {
    QMessageBox::critical(this, tr("Loader error"), tr("There is no supported loader for this file"));
    this->closeDocument();
    return (false);
  }

  // Select arch
    medusa::Architecture::VectorSharedPtr const & archis = this->_medusa.GetAvailableArchitectures();

  // If no compatible arch was found
  if (archis.empty())
  {
    QMessageBox::critical(this, tr("Architecture error"), tr("There is no supported architecture for this file"));
    this->closeDocument();
    return (false);
  }

  medusa::Loader::SharedPtr loader;
  medusa::Architecture::SharedPtr architecture;

  if (!this->_loaderChooser.getSelection(loader, architecture))
  {
    this->closeDocument();
    return (false);
  }

  this->_selectedLoader = loader;
  _medusa.RegisterArchitecture(architecture);

  this->statusbar->showMessage(tr("Interpreting executable format using ") + loader->GetName());
  loader->Map();

  this->statusbar->showMessage(tr("Disassembling ..."));

  this->_medusa.StartAsync(loader, architecture);

  _disasmView.bindMedusa(&_medusa);

  this->actionGoto->setEnabled(true);
  this->_documentOpened = true;
  this->setWindowTitle("Medusa - " + this->_fileName);

  return (true);
}

bool    MainWindow::closeDocument()
{
  this->actionClose->setEnabled(false);
  this->actionGoto->setEnabled(false);

  this->logEdit->clear();
  this->dataList->clear();
  this->codeList->clear();
  this->stringList->clear();
  this->importedList->clear();
  this->exportedList->clear();

  _medusa.Close();

  return true;
}

void MainWindow::updateDisassemblyView(void)
{
  emit disassemblyListingUpdated();
}

void MainWindow::appendLog(std::wstring const& msg)
{
  emit logAppended(QString::fromStdWString(msg));
}

void MainWindow::addLabel(medusa::Label const & label)
{
  emit labelAdded(label);
}

void    MainWindow::on_actionAbout_triggered()
{
  this->_about.exec();
}

void    MainWindow::on_actionOpen_triggered()
{
  // If a document is already opened,
  // Confirmation dialog
  if (this->_documentOpened)
  {
    if (this->_openConfirmation.exec() == QDialog::Rejected)
      return;

    if (!this->closeDocument())
      return;

    this->_openDocument = true;
  }
  this->openDocument();
}

void    MainWindow::on_actionClose_triggered()
{
  this->closeDocument();
}

void    MainWindow::on_actionGoto_triggered()
{
  if (this->_goto.exec() == QDialog::Rejected)
    return;

  if (this->_goto.value() == -1)
    QMessageBox::warning(this, "Input error", "The input text is invalid");
}

void    MainWindow::on_actionSettings_triggered()
{
  this->_settingsDialog.exec();
}

void    MainWindow::_on_label_clicked(QListWidgetItem * item)
{
  medusa::Database & database = this->_medusa.GetDatabase();

  medusa::Address address = database.GetAddressFromLabelName(item->text().toStdString());
  typedef medusa::View::LineInformation LineInformation;
  _disasmView.goTo(LineInformation(LineInformation::LabelLineType, address));
}

void MainWindow::onLogMessageAppended(QString const & msg)
{
  logEdit->insertPlainText(msg);
}

void MainWindow::onLabelAdded(medusa::Label const & label)
{
  QString labelName = QString::fromStdString(label.GetName());

  switch (label.GetType())
  {
  case medusa::Label::LabelData: dataList->addItem(labelName); break;
  case medusa::Label::LabelCode: codeList->addItem(labelName); break;
  case medusa::Label::LabelString: stringList->addItem(labelName); break;
  default: break;
  }

  if (label.GetType() & medusa::Label::LabelImported)
    importedList->addItem(labelName);
  else if (label.GetType() & medusa::Label::LabelExported)
    exportedList->addItem(labelName);
}

void    MainWindow::closeEvent(QCloseEvent * event)
{
    Settings::instance().setValue(WINDOW_LAYOUT, this->saveState());
    Settings::instance().setValue(WINDOW_GEOMETRY, this->saveGeometry());
    event->accept();
}