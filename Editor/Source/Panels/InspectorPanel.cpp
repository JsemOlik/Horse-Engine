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
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

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

  // Mesh Renderer Component
  if (m_SelectedEntity.HasComponent<Horse::MeshRendererComponent>()) {
    auto &mesh = m_SelectedEntity.GetComponent<Horse::MeshRendererComponent>();

    QGroupBox *materialGroup = new QGroupBox("Mesh Renderer");
    QFormLayout *materialLayout = new QFormLayout(materialGroup);

    // Material Selector
    QComboBox *materialCombo = new QComboBox();
    std::vector<std::string> materialNames;
    for (const auto &[name, mat] :
         Horse::MaterialRegistry::Get().GetMaterials()) {
      materialNames.push_back(name);
    }
    std::sort(materialNames.begin(), materialNames.end());

    materialCombo->addItem("None", "");
    int currentIndex = 0;

    for (int i = 0; i < materialNames.size(); ++i) {
      materialCombo->addItem(QString::fromStdString(materialNames[i]),
                             QString::fromStdString(materialNames[i]));
      if (materialNames[i] == mesh.MaterialGUID) {
        currentIndex = i + 1;
      }
    }

    // Handle missing/custom materials
    if (!mesh.MaterialGUID.empty() && currentIndex == 0) {
      bool found = false;
      for (int i = 0; i < materialCombo->count(); ++i) {
        if (materialCombo->itemData(i).toString().toStdString() ==
            mesh.MaterialGUID) {
          currentIndex = i;
          found = true;
          break;
        }
      }

      if (!found) {
        materialCombo->addItem(QString::fromStdString(mesh.MaterialGUID) +
                                   " (Missing)",
                               QString::fromStdString(mesh.MaterialGUID));
        currentIndex = materialCombo->count() - 1;
      }
    }
    materialCombo->setCurrentIndex(currentIndex);

    connect(materialCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, materialCombo](int index) {
              if (m_SelectedEntity) {
                QString guid = materialCombo->itemData(index).toString();
                m_SelectedEntity.GetComponent<Horse::MeshRendererComponent>()
                    .MaterialGUID = guid.toStdString();
                QTimer::singleShot(0, this, [this]() { RefreshInspector(); });
              }
            });

    materialLayout->addRow("Material:", materialCombo);

    // Get Active Material Properties inline
    auto material =
        Horse::MaterialRegistry::Get().GetMaterial(mesh.MaterialGUID);

    if (material) {
      // Albedo Color
      QHBoxLayout *colorLayout = new QHBoxLayout();
      auto color = material->GetColor("Albedo");
      std::array<float, 3> rgb = {color[0], color[1], color[2]};

      for (int i = 0; i < 3; ++i) {
        QDoubleSpinBox *spin = new QDoubleSpinBox();
        spin->setRange(0.0, 1.0);
        spin->setSingleStep(0.01);
        spin->setValue(rgb[i]);

        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, [this, material, i](double val) {
                  if (material) {
                    auto c = material->GetColor("Albedo");
                    c[i] = static_cast<float>(val);
                    material->SetColor("Albedo", c);
                    if (!material->GetFilePath().empty()) {
                      Horse::MaterialSerializer::Serialize(
                          *material, material->GetFilePath());
                    }
                  }
                });
        colorLayout->addWidget(spin);
      }
      materialLayout->addRow("Albedo:", colorLayout);

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

    QPushButton *removeBtn = new QPushButton("Remove Component");
    connect(removeBtn, &QPushButton::clicked, this, [this]() {
      if (m_SelectedEntity) {
        m_SelectedEntity.RemoveComponent<Horse::MeshRendererComponent>();
        RefreshInspector();
      }
    });
    materialLayout->addRow(removeBtn);

    m_ContentLayout->addWidget(materialGroup);
  }

  // Native Script Component
  if (m_SelectedEntity.HasComponent<Horse::NativeScriptComponent>()) {
    auto &script =
        m_SelectedEntity.GetComponent<Horse::NativeScriptComponent>();

    QGroupBox *nsGroup = new QGroupBox("C++ Script");
    QFormLayout *nsLayout = new QFormLayout(nsGroup);

    nsLayout->addRow("Class:",
                     new QLabel(QString::fromStdString(script.ClassName)));

    QPushButton *removeBtn = new QPushButton("Remove Component");
    connect(removeBtn, &QPushButton::clicked, this, [this]() {
      if (m_SelectedEntity) {
        m_SelectedEntity.RemoveComponent<Horse::NativeScriptComponent>();
        RefreshInspector();
      }
    });
    nsLayout->addRow(removeBtn);
    m_ContentLayout->addWidget(nsGroup);
  }

  // Lua Script Component
  if (m_SelectedEntity.HasComponent<Horse::ScriptComponent>()) {
    auto &script = m_SelectedEntity.GetComponent<Horse::ScriptComponent>();

    QGroupBox *lsGroup = new QGroupBox("Lua Script");
    QFormLayout *lsLayout = new QFormLayout(lsGroup);

    // Scan for scripts
    QComboBox *scriptCombo = new QComboBox();
    scriptCombo->addItem("None", "");

    namespace fs = std::filesystem;
    auto assetDir = Horse::Project::GetAssetDirectory();
    auto scriptsDir = assetDir / "Scripts";

    std::vector<std::pair<QString, QString>> availableScripts;

    if (fs::exists(scriptsDir)) {
      for (const auto &entry : fs::recursive_directory_iterator(scriptsDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua") {
          auto relPath =
              fs::relative(entry.path(), Horse::Project::GetProjectDirectory());
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

    if (!script.ScriptPath.empty() && currentIndex == 0) {
      QString currentPath = QString::fromStdString(script.ScriptPath);
      scriptCombo->addItem(currentPath + " (External)", currentPath);
      currentIndex = scriptCombo->count() - 1;
    }

    scriptCombo->setCurrentIndex(currentIndex);

    connect(
        scriptCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this, scriptCombo](int index) {
          if (m_SelectedEntity) {
            QString path = scriptCombo->itemData(index).toString();
            m_SelectedEntity.GetComponent<Horse::ScriptComponent>().ScriptPath =
                path.toStdString();
          }
        });

    lsLayout->addRow("Script:", scriptCombo);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();

    QPushButton *openBtn = new QPushButton("Edit Script");
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
    btnLayout->addWidget(openBtn);

    QPushButton *removeBtn = new QPushButton("Remove Component");
    connect(removeBtn, &QPushButton::clicked, this, [this]() {
      if (m_SelectedEntity) {
        m_SelectedEntity.RemoveComponent<Horse::ScriptComponent>();
        RefreshInspector();
      }
    });
    btnLayout->addWidget(removeBtn);

    lsLayout->addRow(btnLayout);
    m_ContentLayout->addWidget(lsGroup);
  }

  // Camera Component
  if (m_SelectedEntity.HasComponent<Horse::CameraComponent>()) {
    auto &camera = m_SelectedEntity.GetComponent<Horse::CameraComponent>();

    QGroupBox *camGroup = new QGroupBox("Camera");
    QFormLayout *camLayout = new QFormLayout(camGroup);

    // Primary Checkbox
    QCheckBox *primaryCheck = new QCheckBox();
    primaryCheck->setChecked(camera.Primary);
    connect(primaryCheck, &QCheckBox::toggled, this, [this](bool checked) {
      if (m_SelectedEntity)
        m_SelectedEntity.GetComponent<Horse::CameraComponent>().Primary =
            checked;
    });
    camLayout->addRow("Primary:", primaryCheck);

    // Projection Type
    QComboBox *typeCombo = new QComboBox();
    typeCombo->addItem("Perspective");
    typeCombo->addItem("Orthographic");
    typeCombo->setCurrentIndex((int)camera.Type);
    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
              if (m_SelectedEntity) {
                m_SelectedEntity.GetComponent<Horse::CameraComponent>().Type =
                    (Horse::CameraComponent::ProjectionType)index;
                // Refresh to show relevant fields (FOV vs Size)?
                // For now, simple refresh isn't needed if we show all or
                // dynamic update, but usually simpler just to force refresh or
                // hide/show widgets.
                RefreshInspector();
              }
            });
    camLayout->addRow("Projection:", typeCombo);

    if (camera.Type == Horse::CameraComponent::ProjectionType::Perspective) {
      // FOV
      QDoubleSpinBox *fovSpin = new QDoubleSpinBox();
      fovSpin->setRange(1.0, 179.0);
      fovSpin->setValue(camera.FOV);
      connect(fovSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
              this, [this](double val) {
                if (m_SelectedEntity)
                  m_SelectedEntity.GetComponent<Horse::CameraComponent>().FOV =
                      (float)val;
              });
      camLayout->addRow("FOV (Deg):", fovSpin);
    } else {
      // Orthographic Size
      QDoubleSpinBox *sizeSpin = new QDoubleSpinBox();
      sizeSpin->setRange(0.1, 1000.0);
      sizeSpin->setValue(camera.OrthographicSize);
      connect(sizeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
              this, [this](double val) {
                if (m_SelectedEntity)
                  m_SelectedEntity.GetComponent<Horse::CameraComponent>()
                      .OrthographicSize = (float)val;
              });
      camLayout->addRow("Size:", sizeSpin);
    }

    // Near Clip
    QDoubleSpinBox *nearSpin = new QDoubleSpinBox();
    nearSpin->setRange(0.001, 10000.0);
    nearSpin->setValue(camera.NearClip);
    connect(
        nearSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
        [this](double val) {
          if (m_SelectedEntity)
            m_SelectedEntity.GetComponent<Horse::CameraComponent>().NearClip =
                (float)val;
        });
    camLayout->addRow("Near Clip:", nearSpin);

    // Far Clip
    QDoubleSpinBox *farSpin = new QDoubleSpinBox();
    farSpin->setRange(0.01, 100000.0);
    farSpin->setValue(camera.FarClip);
    connect(
        farSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
        [this](double val) {
          if (m_SelectedEntity)
            m_SelectedEntity.GetComponent<Horse::CameraComponent>().FarClip =
                (float)val;
        });
    camLayout->addRow("Far Clip:", farSpin);

    QPushButton *removeBtn = new QPushButton("Remove Component");
    connect(removeBtn, &QPushButton::clicked, this, [this]() {
      if (m_SelectedEntity) {
        m_SelectedEntity.RemoveComponent<Horse::CameraComponent>();
        RefreshInspector();
      }
    });
    camLayout->addRow(removeBtn);

    m_ContentLayout->addWidget(camGroup);
  }

  // RigidBody Component
  if (m_SelectedEntity.HasComponent<Horse::RigidBodyComponent>()) {
    auto &rb = m_SelectedEntity.GetComponent<Horse::RigidBodyComponent>();

    QGroupBox *rbGroup = new QGroupBox("Rigid Body");
    QFormLayout *rbLayout = new QFormLayout(rbGroup);

    QCheckBox *anchoredCheck = new QCheckBox();
    anchoredCheck->setChecked(rb.Anchored);
    connect(anchoredCheck, &QCheckBox::toggled, this, [this](bool checked) {
      if (m_SelectedEntity)
        m_SelectedEntity.GetComponent<Horse::RigidBodyComponent>().Anchored =
            checked;
    });
    rbLayout->addRow("Static (Anchored):", anchoredCheck);

    QCheckBox *gravityCheck = new QCheckBox();
    gravityCheck->setChecked(rb.UseGravity);
    connect(gravityCheck, &QCheckBox::toggled, this, [this](bool checked) {
      if (m_SelectedEntity)
        m_SelectedEntity.GetComponent<Horse::RigidBodyComponent>().UseGravity =
            checked;
    });
    rbLayout->addRow("Use Gravity:", gravityCheck);

    QCheckBox *sensorCheck = new QCheckBox();
    sensorCheck->setChecked(rb.IsSensor);
    connect(sensorCheck, &QCheckBox::toggled, this, [this](bool checked) {
      if (m_SelectedEntity)
        m_SelectedEntity.GetComponent<Horse::RigidBodyComponent>().IsSensor =
            checked;
    });
    rbLayout->addRow("Is Sensor:", sensorCheck);

    // Rotation Locks
    QCheckBox *lockXCheck = new QCheckBox();
    lockXCheck->setChecked(rb.LockRotationX);
    connect(lockXCheck, &QCheckBox::toggled, this, [this](bool checked) {
      if (m_SelectedEntity)
        m_SelectedEntity.GetComponent<Horse::RigidBodyComponent>()
            .LockRotationX = checked;
    });
    rbLayout->addRow("Lock Rotation X:", lockXCheck);

    QCheckBox *lockYCheck = new QCheckBox();
    lockYCheck->setChecked(rb.LockRotationY);
    connect(lockYCheck, &QCheckBox::toggled, this, [this](bool checked) {
      if (m_SelectedEntity)
        m_SelectedEntity.GetComponent<Horse::RigidBodyComponent>()
            .LockRotationY = checked;
    });
    rbLayout->addRow("Lock Rotation Y:", lockYCheck);

    QCheckBox *lockZCheck = new QCheckBox();
    lockZCheck->setChecked(rb.LockRotationZ);
    connect(lockZCheck, &QCheckBox::toggled, this, [this](bool checked) {
      if (m_SelectedEntity)
        m_SelectedEntity.GetComponent<Horse::RigidBodyComponent>()
            .LockRotationZ = checked;
    });
    rbLayout->addRow("Lock Rotation Z:", lockZCheck);

    QPushButton *removeBtn = new QPushButton("Remove Component");
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
      connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
              [this, i](double val) {
                if (m_SelectedEntity)
                  m_SelectedEntity.GetComponent<Horse::BoxColliderComponent>()
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
      connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
              [this, i](double val) {
                if (m_SelectedEntity)
                  m_SelectedEntity.GetComponent<Horse::BoxColliderComponent>()
                      .Offset[i] = (float)val;
              });
      offsetLayout->addWidget(spin);
    }
    bcLayout->addRow(offsetLayout);

    QPushButton *removeBtn = new QPushButton("Remove Component");
    connect(removeBtn, &QPushButton::clicked, this, [this]() {
      if (m_SelectedEntity) {
        m_SelectedEntity.RemoveComponent<Horse::BoxColliderComponent>();
        RefreshInspector();
      }
    });
    bcLayout->addRow(removeBtn);
    m_ContentLayout->addWidget(bcGroup);
  }

  // --------------------------------------------------------------------------
  // Add Component Button
  // --------------------------------------------------------------------------
  // --------------------------------------------------------------------------
  // Add Component Button
  // --------------------------------------------------------------------------
  QPushButton *addComponentBtn = new QPushButton("Add Component");
  addComponentBtn->setStyleSheet("padding: 5px;");

  connect(addComponentBtn, &QPushButton::clicked, this, [this]() {
    QMenu menu;

    // Core Components
    if (!m_SelectedEntity.HasComponent<Horse::MeshRendererComponent>()) {
      menu.addAction("Mesh Renderer", [this]() {
        m_SelectedEntity.AddComponent<Horse::MeshRendererComponent>();
        RefreshInspector();
      });
    }

    menu.addSeparator();

    // Physics
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

    menu.addSeparator();

    // Scripts (Lua)
    if (!m_SelectedEntity.HasComponent<Horse::ScriptComponent>()) {
      menu.addAction("Lua Script", [this]() {
        m_SelectedEntity.AddComponent<Horse::ScriptComponent>();
        RefreshInspector();
      });
    }

    // Scripts (C++)
    if (!m_SelectedEntity.HasComponent<Horse::NativeScriptComponent>()) {
      QMenu *cppMenu = menu.addMenu("C++ Script");

      auto engine = Horse::Engine::Get();
      auto gameModule = engine ? engine->GetGameModule() : nullptr;

      if (gameModule) {
        auto scriptNames = gameModule->GetAvailableScripts();
        for (const auto &name : scriptNames) {
          cppMenu->addAction(
              QString::fromStdString(name), [this, gameModule, name]() {
                if (m_SelectedEntity) {
                  gameModule->CreateScript(name, m_SelectedEntity);
                  RefreshInspector();
                }
              });
        }
        if (scriptNames.empty()) {
          cppMenu->actions().isEmpty();
          cppMenu->addAction("No scripts found")->setEnabled(false);
        }
      } else {
        cppMenu->addAction("Game Module Not Loaded")->setEnabled(false);
      }
    }

    menu.exec(QCursor::pos());
  });

  m_ContentLayout->addWidget(addComponentBtn);
  m_ContentLayout->addStretch();
}
