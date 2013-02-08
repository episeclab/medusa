#ifndef __DISASM_VIEW_HPP__
# define __DISASM_VIEW_HPP__

# include <QtGui>
# include <QAbstractScrollArea>
# include <QPainter>
# include <QResizeEvent>
# include <QPaintEvent>

# include "Settings.hpp"

# include <vector>

# include <medusa/medusa.hpp>
# include <medusa/address.hpp>
# include <medusa/database.hpp>
# include <medusa/instruction.hpp>

class DisassemblyView : public QAbstractScrollArea
{
  Q_OBJECT

public:
  DisassemblyView(QWidget * parent = 0);
  ~DisassemblyView(void);

  bool goTo(medusa::Address const& address);
  bool goTo(medusa::View::LineInformation const & lineInfo);

  void bindMedusa(medusa::Medusa * core);

  void clear(void);

signals:

public slots:
  void setFont(void);
  void listingUpdated(void);
  void updateCursor(void);
  void showContextMenu(QPoint const& pos);

protected:
  virtual void paintEvent(QPaintEvent * evt);
  virtual void mouseMoveEvent(QMouseEvent * evt);
  virtual void mousePressEvent(QMouseEvent * evt);
  virtual void mouseDoubleClickEvent(QMouseEvent * evt);
  virtual void keyPressEvent(QKeyEvent * evt);

private:
  enum LineType
  {
    UnknownLineType,
    CellLineType,
    MultiCellLineType,
    LabelLineType,
    MemoryAreaType,
    EmptyLineType
  };
  void setCursorPosition(QMouseEvent * evt);
  void setCursorPosition(int x, int y);
  void moveCursorPosition(int x, int y);
  void resetSelection(void);
  void setSelection(int x, int y);
  void getSelectedAddresses(medusa::Address::List& addresses);
  void moveSelection(int x, int y);
  void updateScrollbars(void);
  bool convertPositionToAddress(QPoint const & pos, medusa::Address & addr);
  bool convertMouseToAddress(QMouseEvent * evt, medusa::Address & addr);
  void ensureCursorIsVisible(void);

  medusa::Medusa * _core;
  medusa::Database * _db;
  int _xOffset, _yOffset;
  int _wChar, _hChar;
  int _xCursor, _yCursor;
  int _begSelection, _endSelection;
  int _begSelectionOffset, _endSelectionOffset;
  int _addrLen;
  int _lineNo, _lineLen;
  QTimer _cursorTimer; bool _cursorBlink;
  std::vector<QString> _visibleLines;
  medusa::Address _curAddr;

  /* Actions */
};

#endif // !__DISASM_VIEW_HPP__