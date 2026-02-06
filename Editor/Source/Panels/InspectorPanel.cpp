#include "InspectorPanel.h"
#include "HorseEngine/Engine.h"
#include "HorseEngine/Physics/PhysicsComponents.h"
#include "HorseEngine/Project/Project.h"
#include "HorseEngine/Render/Material.h"
#include "HorseEngine/Render/MaterialRegistry.h"
#include "HorseEngine/Render/MaterialSerializer.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/Scene.h"
#include <algorithm>
#include <filesystem>
#include <vector>

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTimer>
#include <QUrl>
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

    // Helper to create a row
    auto addRow = [&](const QString &label, std::function<float(int)> getter,
                      std::function<void(int, float)> setter) {
      QHBoxLayout *rowLayout = new QHBoxLayout();
      rowLayout->addWidget(new QLabel(label));
      for (int i = 0; i < 3; ++i) {
        QDoubleSpinBox *spin = new QDoubleSpinBox();
        spin->setRange(-1000000.0, 1000000.0);
        spin->setSingleStep(0.1);
        spin->setDecimals(3);
        spin->setValue(getter(i));

        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, [this, i, setter](double val) {
                  if (m_SelectedEntity) {
                    setter(i, static_cast<float>(val));
                  }
                });
        rowLayout->addWidget(spin);
      }
      transformVLayout->addLayout(rowLayout);
    };

    addRow(
        "Position", [&](int i) { return transform.Position[i]; },
        [this](int i, float val) {
          m_SelectedEntity.GetComponent<Horse::TransformComponent>()
              .Position[i] = val;
        });

    addRow(
        "Rotation", [&](int i) { return transform.Rotation[i]; },
        [this](int i, float val) {
          m_SelectedEntity.GetComponent<Horse::TransformComponent>()
              .Rotation[i] = val;
        });

    addRow(
        "Scale", [&](int i) { return transform.Scale[i]; },
        [this](int i, float val) {
          m_SelectedEntity.GetComponent<Horse::TransformComponent>().Scale[i] =
              val;
        });

    m_ContentLayout->addWidget(transformGroup);
  }

  // Camera Component
  if (m_SelectedEntity.HasComponent<Horse::CameraComponent>()) {
    auto &camera = m_SelectedEntity.GetComponent<Horse::CameraComponent>();

    QGroupBox *cameraGroup = new QGroupBox("Camera");
    QFormLayout *cameraLayout = new QFormLayout(cameraGroup);

    QComboBox *typeCombo = new QComboBox();
    typeCombo->addItem("Perspective");
    typeCombo->addItem("Orthographic");
    typeCombo->setCurrentIndex(static_cast<int>(camera.Type));
    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
              if (m_SelectedEntity) {
                m_SelectedEntity.GetComponent<Horse::CameraComponent>().Type =
                    static_cast<Horse::CameraComponent::ProjectionType>(index);
              }
            });
    cameraLayout->addRow("Type:", typeCombo);

    QDoubleSpinBox *fovSpin = new QDoubleSpinBox();
    fovSpin->setRange(1.0, 179.0);
    fovSpin->setValue(camera.FOV);
    connect(fovSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double val) {
              if (m_SelectedEntity) {
                m_SelectedEntity.GetComponent<Horse::CameraComponent>().FOV =
                    static_cast<float>(val);
              }
            });
    cameraLayout->addRow("FOV:", fovSpin);

    QDoubleSpinBox *nearSpin = new QDoubleSpinBox();
    nearSpin->setRange(0.001, 1000.0);
    nearSpin->setValue(camera.NearClip);
    connect(
        nearSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
        [this](double val) {
          if (m_SelectedEntity) {
            m_SelectedEntity.GetComponent<Horse::CameraComponent>().NearClip =
                static_cast<float>(val);
          }
        });
    cameraLayout->addRow("Near Clip:", nearSpin);

    QDoubleSpinBox *farSpin = new QDoubleSpinBox();
    farSpin->setRange(0.01, 10000.0);
    farSpin->setValue(camera.FarClip);
    connect(
        farSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
        [this](double val) {
          if (m_SelectedEntity) {
            m_SelectedEntity.GetComponent<Horse::CameraComponent>().FarClip =
                static_cast<float>(val);
          }
        });
    cameraLayout->addRow("Far Clip:", farSpin);

    QCheckBox *primaryCheck = new QCheckBox();
    primaryCheck->setChecked(camera.Primary);
    connect(primaryCheck, &QCheckBox::toggled, this, [this](bool checked) {
      if (m_SelectedEntity) {
        m_SelectedEntity.GetComponent<Horse::CameraComponent>().Primary =
            checked;
      }
    });
    cameraLayout->addRow("Primary:", primaryCheck);

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

  // Mesh Renderer Component (Material Inspector)
  if (m_SelectedEntity.HasComponent<Horse::MeshRendererComponent>()) {
    auto &mesh = m_SelectedEntity.GetComponent<Horse::MeshRendererComponent>();

    QGroupBox *materialGroup = new QGroupBox("Material");
    QFormLayout *materialLayout = new QFormLayout(materialGroup);

    // Check if material is assigned
    if (mesh.MaterialGUID.empty()) {
      QPushButton *addMatBtn = new QPushButton("Add Material");
      connect(addMatBtn, &QPushButton::clicked, this, [this]() {
        QMenu menu;
        auto materials = Horse::MaterialRegistry::Get().GetMaterials();
        std::vector<std::string> names;
        for (const auto &[name, mat] : materials) {
          names.push_back(name);
        }
        std::sort(names.begin(), names.end());

        for (const auto &name : names) {
          menu.addAction(QString::fromStdString(name), [this, name]() {
            if (m_SelectedEntity) {
              m_SelectedEntity.GetComponent<Horse::MeshRendererComponent>()
                  .MaterialGUID = name;
              QTimer::singleShot(0, this, [this]() { RefreshInspector(); });
            }
          });
        }
        menu.exec(QCursor::pos());
      });
      materialLayout->addRow(addMatBtn);
    } else {
      // Material Selector
      QComboBox *materialCombo = new QComboBox();
      std::vector<std::string> materialNames;
      for (const auto &[name, mat] :
           Horse::MaterialRegistry::Get().GetMaterials()) {
        materialNames.push_back(name);
      }
      std::sort(materialNames.begin(), materialNames.end());

      int currentIndex = -1;
      for (int i = 0; i < materialNames.size(); ++i) {
        materialCombo->addItem(QString::fromStdString(materialNames[i]));
        if (materialNames[i] == mesh.MaterialGUID) {
          currentIndex = i;
        }
      }

      // If current material not found in list, add it to end to show it exists
      // but is missing?
      if (currentIndex == -1) {
        materialCombo->addItem(QString::fromStdString(mesh.MaterialGUID) +
                               " (Missing)");
        materialCombo->setCurrentIndex(materialCombo->count() - 1);
      } else {
        materialCombo->setCurrentIndex(currentIndex);
      }

      connect(materialCombo,
              QOverload<int>::of(&QComboBox::currentIndexChanged), this,
              [this, materialNames](int index) {
                if (m_SelectedEntity && index >= 0 &&
                    index < materialNames.size()) {
                  m_SelectedEntity.GetComponent<Horse::MeshRendererComponent>()
                      .MaterialGUID = materialNames[index];
                  QTimer::singleShot(0, this, [this]() { RefreshInspector(); });
                }
              });

      materialLayout->addRow("Assignment:", materialCombo);

      // Get Active Material
      auto material =
          Horse::MaterialRegistry::Get().GetMaterial(mesh.MaterialGUID);

      if (material) {
        // Albedo Color (RGB SpinBoxes for now)
        QHBoxLayout *colorLayout = new QHBoxLayout();
        auto color = material->GetColor("Albedo");
        std::array<float, 3> rgb = {color[0], color[1], color[2]};

        for (int i = 0; i < 3; ++i) {
          QDoubleSpinBox *spin = new QDoubleSpinBox();
          spin->setRange(0.0, 1.0);
          spin->setSingleStep(0.01);
          spin->setValue(rgb[i]);

          // Connect Signal
          connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                  this, [this, material, i](double val) {
                    if (material) {
                      auto c = material->GetColor("Albedo");
                      c[i] = static_cast<float>(val);
                      material->SetColor("Albedo", c);

                      // Save
                      if (!material->GetFilePath().empty()) {
                        Horse::MaterialSerializer::Serialize(
                            *material, material->GetFilePath());
                      }
                    }
                  });
          colorLayout->addWidget(spin);
        }
        materialLayout->addRow("Albedo (RGB):", colorLayout);

        // Roughness
        QDoubleSpinBox *roughnessSpin = new QDoubleSpinBox();
        roughnessSpin->setRange(0.0, 1.0);
        roughnessSpin->setSingleStep(0.01);
        roughnessSpin->setValue(material->GetFloat("Roughness"));
        connect(roughnessSpin,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [this, material](double val) {
                  if (material) {
                    material->SetFloat("Roughness", static_cast<float>(val));
                    if (!material->GetFilePath().empty()) {
                      Horse::MaterialSerializer::Serialize(
                          *material, material->GetFilePath());
                    }
                  }
                });
        materialLayout->addRow("Roughness:", roughnessSpin);

        // Metalness
        QDoubleSpinBox *metalnessSpin = new QDoubleSpinBox();
        metalnessSpin->setRange(0.0, 1.0);
        metalnessSpin->setSingleStep(0.01);
        metalnessSpin->setValue(material->GetFloat("Metalness"));
        connect(metalnessSpin,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [this, material](double val) {
                  if (material) {
                    material->SetFloat("Metalness", static_cast<float>(val));
                    if (!material->GetFilePath().empty()) {
                      Horse::MaterialSerializer::Serialize(
                          *material, material->GetFilePath());
                    }
                  }
                });
        materialLayout->addRow("Metalness:", metalnessSpin);
      }
    }
    m_ContentLayout->addWidget(materialGroup);
  }

  // Native Script Component
  if (m_SelectedEntity.HasComponent<Horse::NativeScriptComponent>()) {
    auto &script =
        m_SelectedEntity.GetComponent<Horse::NativeScriptComponent>();

    QGroupBox *scriptGroup = new QGroupBox("Native Script");
    QFormLayout *scriptLayout = new QFormLayout(scriptGroup);

    scriptLayout->addRow("Class:",
                         new QLabel(QString::fromStdString(script.ClassName)));

    QPushButton *removeBtn = new QPushButton("Remove Script");
    connect(removeBtn, &QPushButton::clicked, this, [this]() {
      if (m_SelectedEntity) {
        m_SelectedEntity.RemoveComponent<Horse::NativeScriptComponent>();
        RefreshInspector();
      }
    });
    scriptLayout->addRow(removeBtn);

    m_ContentLayout->addWidget(scriptGroup);
  } else {
    // Add Component Menu for Scripts
    QPushButton *addScriptBtn = new QPushButton("Attach C++ Script");
    connect(addScriptBtn, &QPushButton::clicked, this, [this]() {
      auto engine = Horse::Engine::Get();
      auto gameModule = engine ? engine->GetGameModule() : nullptr;
      if (!gameModule) {
        HORSE_LOG_CORE_ERROR("No Game Module loaded!");
        return;
      }

      QMenu menu;
      auto scriptNames = gameModule->GetAvailableScripts();
      for (const auto &name : scriptNames) {
        menu.addAction(QString::fromStdString(name),
                       [this, gameModule, name]() {
                         if (m_SelectedEntity) {
                           gameModule->CreateScript(name, m_SelectedEntity);
                           RefreshInspector();
                         }
                       });
      }

      if (scriptNames.empty()) {
        menu.addAction("No scripts available")->setEnabled(false);
      }

      menu.exec(QCursor::pos());
    });
    m_ContentLayout->addWidget(addScriptBtn);

    // Lua Script Component
    if (m_SelectedEntity.HasComponent<Horse::ScriptComponent>()) {
      auto &script = m_SelectedEntity.GetComponent<Horse::ScriptComponent>();
      QGroupBox *group = new QGroupBox("Lua Script", this);
      QFormLayout *layout = new QFormLayout(group);

      // 1. Scan for scripts
      QComboBox *scriptCombo = new QComboBox();
      scriptCombo->addItem("None", "");

      namespace fs = std::filesystem;
      auto assetDir = Horse::Project::GetAssetDirectory();
      auto scriptsDir = assetDir / "Scripts";

      std::vector<std::pair<QString, QString>> availableScripts;

      if (fs::exists(scriptsDir)) {
        for (const auto &entry : fs::recursive_directory_iterator(scriptsDir)) {
          if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            auto relPath = fs::relative(entry.path(),
                                        Horse::Project::GetProjectDirectory());
            QString fullRelPath = QString::fromStdString(relPath.string());
            QString baseName =
                QString::fromStdString(entry.path().stem().string());
            availableScripts.push_back({baseName, fullRelPath});
          }
        }
      }

      std::sort(availableScripts.begin(), availableScripts.end());

      int currentIndex = 0; // "None"
      for (int i = 0; i < availableScripts.size(); ++i) {
        scriptCombo->addItem(availableScripts[i].first,
                             availableScripts[i].second);
        if (availableScripts[i].second.toStdString() == script.ScriptPath) {
          currentIndex = i + 1;
        }
      }

      // If manually set to an absolute path, add it
      if (!script.ScriptPath.empty() && currentIndex == 0) {
        QString currentPath = QString::fromStdString(script.ScriptPath);
        scriptCombo->addItem(currentPath + " (External)", currentPath);
        currentIndex = scriptCombo->count() - 1;
      }

      scriptCombo->setCurrentIndex(currentIndex);

      connect(scriptCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
              this, [this, scriptCombo](int index) {
                if (m_SelectedEntity) {
                  QString path = scriptCombo->itemData(index).toString();
                  m_SelectedEntity.GetComponent<Horse::ScriptComponent>()
                      .ScriptPath = path.toStdString();
                }
              });

      layout->addRow("Script:", scriptCombo);

      // Open Script Button
      QPushButton *openBtn = new QPushButton("Open Script");
      connect(openBtn, &QPushButton::clicked, this, [this, scriptCombo]() {
        QString path =
            scriptCombo->itemData(scriptCombo->currentIndex()).toString();
        if (!path.isEmpty()) {
          namespace fs = std::filesystem;
          fs::path fullPath = path.toStdString();
          if (fullPath.is_relative()) {
            fullPath = Horse::Project::GetProjectDirectory() / fullPath;
          }
          QDesktopServices::openUrl(
              QUrl::fromLocalFile(QString::fromStdString(fullPath.string())));
        }
      });
      layout->addRow(openBtn);

      // Remove Component Button
      QPushButton *removeBtn = new QPushButton("Remove Component");
      connect(removeBtn, &QPushButton::clicked, this, [this]() {
        if (m_SelectedEntity) {
          m_SelectedEntity.RemoveComponent<Horse::ScriptComponent>();
          RefreshInspector();
        }
      });
      layout->addRow(removeBtn);

      m_ContentLayout->addWidget(group);
    } else {
      QPushButton *addLuaBtn = new QPushButton("Attach Lua Script");
      connect(addLuaBtn, &QPushButton::clicked, this, [this]() {
        if (m_SelectedEntity) {
          m_SelectedEntity.AddComponent<Horse::ScriptComponent>();
          RefreshInspector();
        }
      });
      // RigidBody Component
      if (m_SelectedEntity.HasComponent<Horse::RigidBodyComponent>()) {
        auto &rb = m_SelectedEntity.GetComponent<Horse::RigidBodyComponent>();
        QGroupBox *rbGroup = new QGroupBox("Rigid Body");
        QFormLayout *rbLayout = new QFormLayout(rbGroup);

        QCheckBox *anchoredCheck = new QCheckBox();
        anchoredCheck->setChecked(rb.Anchored);
        connect(anchoredCheck, &QCheckBox::toggled, this, [this](bool checked) {
          if (m_SelectedEntity)
            m_SelectedEntity.GetComponent<Horse::RigidBodyComponent>()
                .Anchored = checked;
        });
        rbLayout->addRow("Anchored (Static):", anchoredCheck);

        QCheckBox *gravityCheck = new QCheckBox();
        gravityCheck->setChecked(rb.UseGravity);
        connect(gravityCheck, &QCheckBox::toggled, this, [this](bool checked) {
          if (m_SelectedEntity)
            m_SelectedEntity.GetComponent<Horse::RigidBodyComponent>()
                .UseGravity = checked;
        });
        rbLayout->addRow("Use Gravity:", gravityCheck);

        QCheckBox *sensorCheck = new QCheckBox();
        sensorCheck->setChecked(rb.IsSensor);
        connect(sensorCheck, &QCheckBox::toggled, this, [this](bool checked) {
          if (m_SelectedEntity)
            m_SelectedEntity.GetComponent<Horse::RigidBodyComponent>()
                .IsSensor = checked;
        });
        rbLayout->addRow("Is Sensor:", sensorCheck);

        // Remove Button
        QPushButton *removeBtn = new QPushButton("Remove RigidBody");
        connect(removeBtn, &QPushButton::clicked, this, [this]() {
          if (m_SelectedEntity) {
            m_SelectedEntity.RemoveComponent<Horse::RigidBodyComponent>();
            RefreshInspector();
          }
        });
        rbLayout->addRow(removeBtn);

        m_ContentLayout->addWidget(rbGroup);
      }

      // BoxCollider Component
      if (m_SelectedEntity.HasComponent<Horse::BoxColliderComponent>()) {
        auto &bc = m_SelectedEntity.GetComponent<Horse::BoxColliderComponent>();
        QGroupBox *bcGroup = new QGroupBox("Box Collider");
        QFormLayout *bcLayout = new QFormLayout(bcGroup);

        // Size
        QHBoxLayout *sizeLayout = new QHBoxLayout();
        sizeLayout->addWidget(new QLabel("Size"));
        for (int i = 0; i < 3; ++i) {
          QDoubleSpinBox *spin = new QDoubleSpinBox();
          spin->setRange(0.01, 10000.0);
          spin->setValue(bc.Size[i]);
          connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                  this, [this, i](double val) {
                    if (m_SelectedEntity)
                      m_SelectedEntity
                          .GetComponent<Horse::BoxColliderComponent>()
                          .Size[i] = (float)val;
                  });
          sizeLayout->addWidget(spin);
        }
        bcLayout->addRow(sizeLayout);

        // Offset
        QHBoxLayout *offsetLayout = new QHBoxLayout();
        offsetLayout->addWidget(new QLabel("Offset"));
        for (int i = 0; i < 3; ++i) {
          QDoubleSpinBox *spin = new QDoubleSpinBox();
          spin->setRange(-10000.0, 10000.0);
          spin->setValue(bc.Offset[i]);
          connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                  this, [this, i](double val) {
                    if (m_SelectedEntity)
                      m_SelectedEntity
                          .GetComponent<Horse::BoxColliderComponent>()
                          .Offset[i] = (float)val;
                  });
          offsetLayout->addWidget(spin);
        }
        bcLayout->addRow(offsetLayout);

        // Remove Button
        QPushButton *removeBtn = new QPushButton("Remove BoxCollider");
        connect(removeBtn, &QPushButton::clicked, this, [this]() {
          if (m_SelectedEntity) {
            m_SelectedEntity.RemoveComponent<Horse::BoxColliderComponent>();
            RefreshInspector();
          }
        });
        bcLayout->addRow(removeBtn);

        m_ContentLayout->addWidget(bcGroup);
      }

      // General Add Component Button
      QPushButton *addComponentBtn = new QPushButton("Add Component");
      connect(addComponentBtn, &QPushButton::clicked, this, [this]() {
        QMenu menu;
        if (m_SelectedEntity) {
          if (!m_SelectedEntity.HasComponent<Horse::RigidBodyComponent>()) {
            menu.addAction("Rigid Body", [this]() {
              m_SelectedEntity.AddComponent<Horse::RigidBodyComponent>();
              RefreshInspector();
            });
          }
          if (!m_SelectedEntity.HasComponent<Horse::BoxColliderComponent>()) {
            menu.addAction("Box Collider", [this]() {
              m_SelectedEntity.AddComponent<Horse::BoxColliderComponent>();
              RefreshInspector();
            });
          }
          // Can add other missing components here if needed
        }
        menu.exec(QCursor::pos());
      });
      m_ContentLayout->addWidget(addComponentBtn);
    }
