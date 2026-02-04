#include "InspectorPanel.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Entity.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

InspectorPanel::InspectorPanel(QWidget *parent) : QWidget(parent) {

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  m_ScrollArea = new QScrollArea(this);
  m_ScrollArea->setWidgetResizable(true);
  m_ScrollArea->setFrameShape(QFrame::NoFrame);

  m_ContentWidget = new QWidget();
  m_ContentLayout = new QVBoxLayout(m_ContentWidget);
  m_ContentLayout->setAlignment(Qt::AlignTop);

  m_ScrollArea->setWidget(m_ContentWidget);
  mainLayout->addWidget(m_ScrollArea);

  RefreshInspector();
}

void InspectorPanel::SetSelectedEntity(Horse::Entity entity) {
  m_SelectedEntity = entity;
  RefreshInspector();
}

void InspectorPanel::RefreshInspector() {
  // Clear existing widgets
  QLayoutItem *item;
  while ((item = m_ContentLayout->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }

  if (!m_SelectedEntity) {
    QLabel *placeholder = new QLabel("Select an entity to inspect");
    placeholder->setAlignment(Qt::AlignCenter);
    m_ContentLayout->addWidget(placeholder);
    return;
  }

  DrawComponents();
}

void InspectorPanel::DrawComponents() {
  // Tag Component
  if (m_SelectedEntity.HasComponent<Horse::TagComponent>()) {
    auto &tag = m_SelectedEntity.GetComponent<Horse::TagComponent>();

    QGroupBox *tagGroup = new QGroupBox("Tag");
    QFormLayout *tagLayout = new QFormLayout(tagGroup);

    QLineEdit *nameEdit = new QLineEdit(QString::fromStdString(tag.Name));
    connect(nameEdit, &QLineEdit::textChanged, this,
            [this](const QString &text) {
              if (m_SelectedEntity) {
                m_SelectedEntity.GetComponent<Horse::TagComponent>().Name =
                    text.toStdString();
              }
            });
    tagLayout->addRow("Name:", nameEdit);

    QLineEdit *tagEdit = new QLineEdit(QString::fromStdString(tag.Tag));
    connect(tagEdit, &QLineEdit::textChanged, this,
            [this](const QString &text) {
              if (m_SelectedEntity) {
                m_SelectedEntity.GetComponent<Horse::TagComponent>().Tag =
                    text.toStdString();
              }
            });
    tagLayout->addRow("Tag:", tagEdit);

    m_ContentLayout->addWidget(tagGroup);
  }

  // Transform Component
  if (m_SelectedEntity.HasComponent<Horse::TransformComponent>()) {
    auto &transform =
        m_SelectedEntity.GetComponent<Horse::TransformComponent>();

    QGroupBox *transformGroup = new QGroupBox("Transform");
    QVBoxLayout *transformVLayout = new QVBoxLayout(transformGroup);

    auto createVec3Control = [&](const QString &label,
                                 std::array<float, 3> &values) {
      QHBoxLayout *rowLayout = new QHBoxLayout();
      rowLayout->addWidget(new QLabel(label));

      for (int i = 0; i < 3; ++i) {
        QDoubleSpinBox *spin = new QDoubleSpinBox();
        spin->setRange(-1000000.0, 1000000.0);
        spin->setSingleStep(0.1);
        spin->setValue(values[i]);
        spin->setDecimals(3);

        // Re-implementing connection to capture index
        auto updateFunc = [this, &values, i](double val) {
          if (m_SelectedEntity) {
            values[i] = static_cast<float>(val);
          }
        };
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, updateFunc);

        rowLayout->addWidget(spin);
      }
      transformVLayout->addLayout(rowLayout);
    };

    createVec3Control("Position", transform.Position);
    createVec3Control("Rotation", transform.Rotation);
    createVec3Control("Scale", transform.Scale);

    m_ContentLayout->addWidget(transformGroup);
  }

  // Camera Component
  if (m_SelectedEntity.HasComponent<Horse::CameraComponent>()) {
    auto &camera = m_SelectedEntity.GetComponent<Horse::CameraComponent>();

    QGroupBox *cameraGroup = new QGroupBox("Camera");
    QFormLayout *cameraLayout = new QFormLayout(cameraGroup);

    cameraLayout->addRow(
        "Type:",
        new QLabel(camera.Type ==
                           Horse::CameraComponent::ProjectionType::Perspective
                       ? "Perspective"
                       : "Orthographic"));
    cameraLayout->addRow("FOV:", new QLabel(QString::number(camera.FOV)));
    cameraLayout->addRow("Near Clip:",
                         new QLabel(QString::number(camera.NearClip)));
    cameraLayout->addRow("Far Clip:",
                         new QLabel(QString::number(camera.FarClip)));
    cameraLayout->addRow("Primary:", new QLabel(camera.Primary ? "Yes" : "No"));

    m_ContentLayout->addWidget(cameraGroup);
  }

  // Light Component
  if (m_SelectedEntity.HasComponent<Horse::LightComponent>()) {
    auto &light = m_SelectedEntity.GetComponent<Horse::LightComponent>();

    QGroupBox *lightGroup = new QGroupBox("Light");
    QFormLayout *lightLayout = new QFormLayout(lightGroup);

    QString typeStr;
    switch (light.Type) {
    case Horse::LightComponent::LightType::Directional:
      typeStr = "Directional";
      break;
    case Horse::LightComponent::LightType::Point:
      typeStr = "Point";
      break;
    case Horse::LightComponent::LightType::Spot:
      typeStr = "Spot";
      break;
    }

    lightLayout->addRow("Type:", new QLabel(typeStr));
    lightLayout->addRow("Color:", new QLabel(QString("R: %1, G: %2, B: %3")
                                                 .arg(light.Color[0])
                                                 .arg(light.Color[1])
                                                 .arg(light.Color[2])));
    lightLayout->addRow("Intensity:",
                        new QLabel(QString::number(light.Intensity)));

    m_ContentLayout->addWidget(lightGroup);
  }
}
