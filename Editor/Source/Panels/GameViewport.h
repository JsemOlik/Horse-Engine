#pragma once

#include "D3D11ViewportWidget.h"

class GameViewport : public D3D11ViewportWidget {
  Q_OBJECT

public:
  explicit GameViewport(QWidget *parent = nullptr);

protected:
  void Render() override;
};
