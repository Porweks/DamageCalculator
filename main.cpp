#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QQueue>
#include <QMap>
#include <QVector>
#include <QDir>
#include <QStandardPaths>
#include <algorithm>

// Структура оружия
struct Weapon {
    QString name = "Стандартное";
    double dmg = 301.25;
    double decr = 27.5;
    double aspd = 40.0;
    bool module_same = true;
    double module_mult = 1.089;
    double module_mult_head = 1.089;
    double module_mult_body = 1.089;
    bool exec_enabled = false;
    double exec_per_pct = 0.0;
};

// Структура группы выстрелов (UI)
struct ShotGroupUI {
    QWidget* widget;
    QLineEdit* countEdit;
    QComboBox* zoneCombo;
    QComboBox* weaponCombo;
    QCheckBox* moduleCheck;
};

// Структура события выстрела (для симуляции)
struct ShotEvent {
    int groupIndex;
    QString zone;
    bool useModule;
    Weapon weapon;
};

class DamageCalculator : public QWidget {
    Q_OBJECT

public:
    DamageCalculator(QWidget* parent = nullptr) : QWidget(parent) {
        setWindowTitle("Калькулятор урона (C++ / Qt)");
        resize(1350, 950);

        weapons = loadWeapons();
        currentWeaponIdx = 0;
        loadingWeapon = false;

        QHBoxLayout* mainLayout = new QHBoxLayout(this);

        // Левая панель
        QVBoxLayout* leftLayout = new QVBoxLayout();
        buildTargetPanel(leftLayout);
        buildWeaponPanel(leftLayout);
        buildShotsPanel(leftLayout);
        mainLayout->addLayout(leftLayout, 0);

        // Правая панель (Результаты)
        buildResultsPanel(mainLayout);

        refreshWeaponCombobox();
        loadWeaponIntoFields(0);
        addShotGroup();
        calculate();

        setStyleSheet(
            "QGroupBox { font-weight: bold; margin-top: 1ex; }"
            "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 3px; color: #569cd6; }"
            "QTextEdit { background-color: #1e1e1e; color: #d4d4d4; font-family: Consolas, monospace; }"
        );
    }

private:
    QVector<Weapon> weapons;
    int currentWeaponIdx;
    bool loadingWeapon;

    // Поля цели
    QLineEdit* baseHpEdit, * multEdit, * regenEdit, * headMultEdit, * bodyMultEdit;

    // Поля оружия
    QComboBox* weaponSelector;
    QLineEdit* wNameEdit, * wDmgEdit, * wDecrEdit, * wAspdEdit;
    QRadioButton* modSameRadio, * modDiffRadio;
    QLineEdit* modSameEdit, * modHeadEdit, * modBodyEdit;
    QCheckBox* execEnabledCheck;
    QLineEdit* execPerPctEdit;

    // Выстрелы
    QVBoxLayout* shotsContainerLayout;
    QVector<ShotGroupUI> shotGroups;

    // Результаты
    QTextEdit* resultText;

    // ----- Загрузка / Сохранение -----
    QString getWeaponsFilePath() {
        return QDir(QCoreApplication::applicationDirPath()).filePath("weapons.json");
    }

    QVector<Weapon> loadWeapons() {
        QVector<Weapon> result;
        QFile file(getWeaponsFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isArray()) {
                for (const QJsonValue& val : doc.array()) {
                    QJsonObject obj = val.toObject();
                    Weapon w;
                    w.name = obj["name"].toString("Без имени");
                    w.dmg = obj["dmg"].toDouble(0.0);
                    w.decr = obj["decr"].toDouble(0.0);
                    w.aspd = obj["aspd"].toDouble(60.0);
                    w.module_same = obj["module_same"].toBool(true);
                    w.module_mult = obj["module_mult"].toDouble(1.0);
                    w.module_mult_head = obj["module_mult_head"].toDouble(1.0);
                    w.module_mult_body = obj["module_mult_body"].toDouble(1.0);
                    w.exec_enabled = obj["exec_enabled"].toBool(false);
                    w.exec_per_pct = obj["exec_per_pct"].toDouble(0.0);
                    result.append(w);
                }
            }
        }
        if (result.isEmpty()) {
            Weapon defaultW;
            result.append(defaultW);
        }
        return result;
    }

    void saveWeapons() {
        QJsonArray arr;
        for (const Weapon& w : weapons) {
            QJsonObject obj;
            obj["name"] = w.name;
            obj["dmg"] = w.dmg;
            obj["decr"] = w.decr;
            obj["aspd"] = w.aspd;
            obj["module_same"] = w.module_same;
            obj["module_mult"] = w.module_mult;
            obj["module_mult_head"] = w.module_mult_head;
            obj["module_mult_body"] = w.module_mult_body;
            obj["exec_enabled"] = w.exec_enabled;
            obj["exec_per_pct"] = w.exec_per_pct;
            arr.append(obj);
        }
        QFile file(getWeaponsFilePath());
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        }
    }

    // ----- UI Построение -----
    void buildTargetPanel(QVBoxLayout* parentLayout) {
        QGroupBox* group = new QGroupBox("Цель");
        QFormLayout* layout = new QFormLayout(group);

        baseHpEdit = new QLineEdit("0");
        multEdit = new QLineEdit("0");
        regenEdit = new QLineEdit("0");
        headMultEdit = new QLineEdit("1.4");
        bodyMultEdit = new QLineEdit("0.8");

        layout->addRow("Пулестой (HP):", baseHpEdit);
        layout->addRow("Живка (%):", multEdit);
        layout->addRow("Отхилл (%/сек):", regenEdit);
        layout->addRow("Множитель голова:", headMultEdit);
        layout->addRow("Множитель корпус:", bodyMultEdit);

        parentLayout->addWidget(group);
    }

    void buildWeaponPanel(QVBoxLayout* parentLayout) {
        QGroupBox* group = new QGroupBox("Оружие (пресеты)");
        QVBoxLayout* mainLay = new QVBoxLayout(group);

        QHBoxLayout* topRow = new QHBoxLayout();
        weaponSelector = new QComboBox();
        topRow->addWidget(new QLabel("Пресет:"));
        topRow->addWidget(weaponSelector, 1);

        QPushButton* btnAdd = new QPushButton("+");
        QPushButton* btnCopy = new QPushButton("⧉");
        QPushButton* btnDel = new QPushButton("×");
        btnAdd->setFixedWidth(30); btnCopy->setFixedWidth(30); btnDel->setFixedWidth(30);
        topRow->addWidget(btnAdd);
        topRow->addWidget(btnCopy);
        topRow->addWidget(btnDel);
        mainLay->addLayout(topRow);

        QFormLayout* form = new QFormLayout();
        wNameEdit = new QLineEdit();
        wDmgEdit = new QLineEdit();
        wDecrEdit = new QLineEdit();
        wAspdEdit = new QLineEdit();

        form->addRow("Название:", wNameEdit);
        form->addRow("Урон:", wDmgEdit);
        form->addRow("Бронебой (%):", wDecrEdit);
        form->addRow("Скорострельность (уд/мин):", wAspdEdit);
        mainLay->addLayout(form);

        // Модули
        QGroupBox* modGroup = new QGroupBox("Модуль урона");
        QVBoxLayout* modLay = new QVBoxLayout(modGroup);

        QHBoxLayout* radioLay = new QHBoxLayout();
        modSameRadio = new QRadioButton("Одинаковый");
        modDiffRadio = new QRadioButton("Разный по зонам");
        modSameRadio->setChecked(true);
        radioLay->addWidget(modSameRadio);
        radioLay->addWidget(modDiffRadio);
        modLay->addLayout(radioLay);

        QFormLayout* modForm = new QFormLayout();
        modSameEdit = new QLineEdit();
        modHeadEdit = new QLineEdit();
        modBodyEdit = new QLineEdit();
        modForm->addRow("module_mult:", modSameEdit);
        modForm->addRow("голова:", modHeadEdit);
        modForm->addRow("корпус:", modBodyEdit);
        modLay->addLayout(modForm);

        mainLay->addWidget(modGroup);

        QGroupBox* execGroup = new QGroupBox("Модуль «Добивание»");
        QVBoxLayout* execLay = new QVBoxLayout(execGroup);
        execEnabledCheck = new QCheckBox("Включён");
        QFormLayout* execForm = new QFormLayout();
        execPerPctEdit = new QLineEdit();
        execForm->addRow("+X% за 1% недост. HP:", execPerPctEdit);
        execLay->addWidget(execEnabledCheck);
        execLay->addLayout(execForm);
        mainLay->addWidget(execGroup);

        parentLayout->addWidget(group);

        // Коннекты оружия
        connect(weaponSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DamageCalculator::onWeaponSelect);

        auto updateWeapon = [this]() { onWeaponFieldChange(); };
        connect(wNameEdit, &QLineEdit::textChanged, this, updateWeapon);
        connect(wDmgEdit, &QLineEdit::textChanged, this, updateWeapon);
        connect(wDecrEdit, &QLineEdit::textChanged, this, updateWeapon);
        connect(wAspdEdit, &QLineEdit::textChanged, this, updateWeapon);
        connect(modSameEdit, &QLineEdit::textChanged, this, updateWeapon);
        connect(modHeadEdit, &QLineEdit::textChanged, this, updateWeapon);
        connect(modBodyEdit, &QLineEdit::textChanged, this, updateWeapon);
        connect(execPerPctEdit, &QLineEdit::textChanged, this, updateWeapon);

        connect(modSameRadio, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                updateWeaponModuleFields();
                onWeaponFieldChange();
            }
            });
        connect(execEnabledCheck, &QCheckBox::toggled, this, [this](bool) { onWeaponFieldChange(); });

        connect(btnAdd, &QPushButton::clicked, this, &DamageCalculator::addWeapon);
        connect(btnCopy, &QPushButton::clicked, this, &DamageCalculator::copyWeapon);
        connect(btnDel, &QPushButton::clicked, this, &DamageCalculator::deleteWeapon);
    }

    void buildShotsPanel(QVBoxLayout* parentLayout) {
        QGroupBox* group = new QGroupBox("Выстрелы");
        QVBoxLayout* layout = new QVBoxLayout(group);

        QHBoxLayout* header = new QHBoxLayout();
        auto addHdr = [&](QString txt, int w) {
            QLabel* l = new QLabel(txt); l->setFixedWidth(w); header->addWidget(l);
            };
        addHdr("#", 25); addHdr("кол-во", 50); addHdr("зона", 60); addHdr("оружие", 140); addHdr("модуль", 50);
        layout->addLayout(header);

        QScrollArea* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        QWidget* scrollContent = new QWidget();
        shotsContainerLayout = new QVBoxLayout(scrollContent);
        shotsContainerLayout->setAlignment(Qt::AlignTop);
        shotsContainerLayout->setContentsMargins(0, 0, 0, 0);
        scroll->setWidget(scrollContent);
        layout->addWidget(scroll);

        QHBoxLayout* btnRow = new QHBoxLayout();
        QPushButton* btnAddShot = new QPushButton("+ Добавить группу");
        QPushButton* btnCalc = new QPushButton("Рассчитать");
        btnRow->addWidget(btnAddShot);
        btnRow->addStretch();
        btnRow->addWidget(btnCalc);
        layout->addLayout(btnRow);

        parentLayout->addWidget(group, 1);

        connect(btnAddShot, &QPushButton::clicked, this, &DamageCalculator::addShotGroup);
        connect(btnCalc, &QPushButton::clicked, this, &DamageCalculator::calculate);
    }

    void buildResultsPanel(QHBoxLayout* parentLayout) {
        QGroupBox* group = new QGroupBox("Результаты");
        QVBoxLayout* layout = new QVBoxLayout(group);
        resultText = new QTextEdit();
        resultText->setReadOnly(true);
        resultText->setLineWrapMode(QTextEdit::NoWrap);
        layout->addWidget(resultText);
        parentLayout->addWidget(group, 1);
    }

    // ----- Логика UI -----
    void updateWeaponModuleFields() {
        bool same = modSameRadio->isChecked();
        modSameEdit->setEnabled(same);
        modHeadEdit->setEnabled(!same);
        modBodyEdit->setEnabled(!same);
    }

    void onWeaponSelect(int idx) {
        if (idx >= 0 && idx < weapons.size()) {
            currentWeaponIdx = idx;
            loadWeaponIntoFields(idx);
        }
    }

    void loadWeaponIntoFields(int idx) {
        loadingWeapon = true;
        Weapon& w = weapons[idx];
        wNameEdit->setText(w.name);
        wDmgEdit->setText(QString::number(w.dmg));
        wDecrEdit->setText(QString::number(w.decr));
        wAspdEdit->setText(QString::number(w.aspd));
        modSameEdit->setText(QString::number(w.module_mult));
        modHeadEdit->setText(QString::number(w.module_mult_head));
        modBodyEdit->setText(QString::number(w.module_mult_body));
        execPerPctEdit->setText(QString::number(w.exec_per_pct));
        modSameRadio->setChecked(w.module_same);
        modDiffRadio->setChecked(!w.module_same);
        execEnabledCheck->setChecked(w.exec_enabled);
        updateWeaponModuleFields();
        loadingWeapon = false;
    }

    void onWeaponFieldChange() {
        if (loadingWeapon) return;
        int idx = currentWeaponIdx;
        if (idx < 0 || idx >= weapons.size()) return;

        Weapon& w = weapons[idx];
        w.name = wNameEdit->text().trimmed();
        if (w.name.isEmpty()) w.name = "Без имени";
        w.dmg = wDmgEdit->text().toDouble();
        w.decr = wDecrEdit->text().toDouble();
        w.aspd = wAspdEdit->text().toDouble();
        w.module_mult = modSameEdit->text().toDouble();
        w.module_mult_head = modHeadEdit->text().toDouble();
        w.module_mult_body = modBodyEdit->text().toDouble();
        w.exec_per_pct = execPerPctEdit->text().toDouble();
        w.module_same = modSameRadio->isChecked();
        w.exec_enabled = execEnabledCheck->isChecked();

        refreshWeaponCombobox();
        saveWeapons();
    }

    void refreshWeaponCombobox() {
        loadingWeapon = true;
        QStringList names;
        for (const auto& w : weapons) names << w.name;

        weaponSelector->clear();
        weaponSelector->addItems(names);
        if (currentWeaponIdx >= 0 && currentWeaponIdx < names.size()) {
            weaponSelector->setCurrentIndex(currentWeaponIdx);
        }
        loadingWeapon = false;

        for (auto& s : shotGroups) {
            QString cur = s.weaponCombo->currentText();
            s.weaponCombo->clear();
            s.weaponCombo->addItems(names);
            int idx = s.weaponCombo->findText(cur);
            if (idx >= 0) s.weaponCombo->setCurrentIndex(idx);
            else if (s.weaponCombo->count() > 0) s.weaponCombo->setCurrentIndex(0);
        }
    }

    void addWeapon() {
        Weapon newW;
        newW.name = QString("Оружие %1").arg(weapons.size() + 1);
        newW.dmg = 100.0; newW.decr = 0.0; newW.aspd = 60.0;
        weapons.append(newW);
        currentWeaponIdx = weapons.size() - 1;
        refreshWeaponCombobox();
        loadWeaponIntoFields(currentWeaponIdx);
        saveWeapons();
    }

    void copyWeapon() {
        if (weapons.isEmpty()) return;
        Weapon newW = weapons[currentWeaponIdx];
        newW.name += " (копия)";
        weapons.append(newW);
        currentWeaponIdx = weapons.size() - 1;
        refreshWeaponCombobox();
        loadWeaponIntoFields(currentWeaponIdx);
        saveWeapons();
    }

    void deleteWeapon() {
        if (weapons.size() <= 1) {
            QMessageBox::warning(this, "Ошибка", "Должно остаться хотя бы одно оружие.");
            return;
        }
        weapons.remove(currentWeaponIdx);
        currentWeaponIdx = std::max(0, currentWeaponIdx - 1);
        refreshWeaponCombobox();
        loadWeaponIntoFields(currentWeaponIdx);
        saveWeapons();
    }

    void addShotGroup() {
        ShotGroupUI s;
        s.widget = new QWidget();
        QHBoxLayout* lay = new QHBoxLayout(s.widget);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(4);

        QLabel* lblIdx = new QLabel(QString::number(shotGroups.size() + 1));
        lblIdx->setFixedWidth(25);
        s.countEdit = new QLineEdit("1"); s.countEdit->setFixedWidth(40);
        s.zoneCombo = new QComboBox(); s.zoneCombo->addItems({ "head", "body", "miss" }); s.zoneCombo->setFixedWidth(60); s.zoneCombo->setCurrentText("body");
        s.weaponCombo = new QComboBox(); s.weaponCombo->setFixedWidth(140);
        for (const auto& w : weapons) s.weaponCombo->addItem(w.name);
        s.moduleCheck = new QCheckBox(); s.moduleCheck->setChecked(true);

        QPushButton* btnDel = new QPushButton("×"); btnDel->setFixedWidth(30);

        lay->addWidget(lblIdx);
        lay->addWidget(s.countEdit);
        lay->addWidget(s.zoneCombo);
        lay->addWidget(s.weaponCombo);
        lay->addWidget(s.moduleCheck);
        lay->addWidget(btnDel);
        lay->addStretch();

        shotsContainerLayout->addWidget(s.widget);
        shotGroups.append(s);

        connect(btnDel, &QPushButton::clicked, this, [this, s, lay]() {
            if (shotGroups.size() <= 1) return;
            shotGroups.removeOne(s);
            delete s.widget;
            // Перенумерация
            for (int i = 0; i < shotGroups.size(); ++i) {
                QLabel* l = qobject_cast<QLabel*>(shotGroups[i].widget->layout()->itemAt(0)->widget());
                if (l) l->setText(QString::number(i + 1));
            }
            });
    }

    // ----- Вывод в консоль (с цветом) -----
    void log(const QString& text, const QColor& color = Qt::black) {
        QTextCharFormat fmt;
        fmt.setForeground(color);
        QTextCursor cursor = resultText->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(text + "\n", fmt);
    }

    // ----- Симуляция -----
    void calculate() {
        resultText->clear();
        QColor cHeader("#569cd6"), cGood("#4ec9b0"), cWarn("#dcdcaa"), cBad("#f44747"), cDim("#808080");

        double base_hp, mult, regen, head_mult, body_mult;
        try {
            base_hp = baseHpEdit->text().toDouble() + 100.0;
            mult = multEdit->text().toDouble() / 100.0;
            regen = regenEdit->text().toDouble() / 100.0;
            head_mult = headMultEdit->text().toDouble();
            body_mult = bodyMultEdit->text().toDouble();
        }
        catch (...) {
            log("Ошибка: проверьте ввод чисел!", cBad);
            return;
        }

        for (const auto& w : weapons) {
            if (w.aspd <= 0) { log(QString("У оружия «%1» скорострельность ≤ 0.").arg(w.name), cBad); return; }
            if (w.dmg < 0) { log(QString("У оружия «%1» урон < 0.").arg(w.name), cBad); return; }
        }

        double hp_full = base_hp * (1 + mult);
        if (hp_full <= 0) { log("HP цели ≤ 0.", cBad); return; }

        auto pen_mult = [&](const Weapon& w) {
            double d = w.decr / 100.0;
            double denom = 1 + mult - d;
            if (denom <= 0) return 1e9;
            return (1 + mult) / denom;
            };

        auto calc_shot_dmg = [&](QString zone, bool useModule, const Weapon& w, double cur_hp) {
            if (zone == "miss") return 0.0;
            double d = w.dmg * pen_mult(w);
            if (useModule) {
                if (w.module_same) d *= w.module_mult;
                else d *= (zone == "head") ? w.module_mult_head : w.module_mult_body;

                if (w.exec_enabled) {
                    double missing = std::max(0.0, (1.0 - cur_hp / hp_full) * 100.0);
                    d *= 1.0 + missing * (w.exec_per_pct / 100.0);
                }
            }
            d *= (zone == "head") ? head_mult : body_mult;
            return d;
            };

        // Сборка очередей по оружию
        QMap<QString, QQueue<ShotEvent>> weaponQueues;
        QMap<QString, double> weaponNextT;
        QMap<QString, double> weaponInterval;
        QStringList weaponOrder;

        for (int gi = 0; gi < shotGroups.size(); ++gi) {
            auto& s = shotGroups[gi];
            int count = s.countEdit->text().toInt();
            if (count <= 0) continue;
            QString zone = s.zoneCombo->currentText();
            bool useModule = s.moduleCheck->isChecked() && zone != "miss";
            QString wname = s.weaponCombo->currentText();

            int wIdx = -1;
            for (int i = 0; i < weapons.size(); ++i) if (weapons[i].name == wname) { wIdx = i; break; }
            if (wIdx < 0) continue;

            Weapon w = weapons[wIdx];
            if (!weaponQueues.contains(wname)) {
                weaponQueues[wname] = QQueue<ShotEvent>();
                weaponNextT[wname] = 0.0;
                weaponInterval[wname] = 60.0 / w.aspd;
                weaponOrder.append(wname);
            }
            for (int i = 0; i < count; ++i) {
                weaponQueues[wname].enqueue({ gi, zone, useModule, w });
            }
        }

        bool hasShots = false;
        for (const auto& q : weaponQueues) if (!q.isEmpty()) hasShots = true;
        if (!hasShots) { log("Добавьте хотя бы один выстрел (кол-во > 0).", cBad); return; }

        // Симуляция
        double cur_hp = hp_full, prev_t = 0.0, regen_ps = hp_full * regen, total_dmg = 0.0;
        int killed_at = -1;
        double kill_time = 0;

        struct LogEntry { int n; int gi; QString zone; bool mod; Weapon w; double dmg; double hp; double cum; double t; };
        QVector<LogEntry> logData;
        int shotNum = 1;

        while (true) {
            QString activeW = "";
            double minT = 1e18;

            // Выбор оружия с минимальным временем
            for (const QString& wname : weaponOrder) {
                if (!weaponQueues[wname].isEmpty()) {
                    double t = weaponNextT[wname];
                    if (t < minT - 0.00001) {
                        minT = t;
                        activeW = wname;
                    }
                    else if (std::abs(t - minT) < 0.00001) {
                        // Tie-break по порядку в weaponOrder
                        if (weaponOrder.indexOf(wname) < weaponOrder.indexOf(activeW)) {
                            activeW = wname;
                        }
                    }
                }
            }
            if (activeW.isEmpty()) break;

            double t_event = weaponNextT[activeW];
            ShotEvent ev = weaponQueues[activeW].dequeue();

            double dt = t_event - prev_t;
            if (dt > 0) cur_hp += regen_ps * dt;
            prev_t = t_event;

            double d_this = calc_shot_dmg(ev.zone, ev.useModule, ev.weapon, cur_hp);
            cur_hp -= d_this;
            total_dmg += d_this;

            logData.append({ shotNum, ev.groupIndex, ev.zone, ev.useModule, ev.weapon, d_this, cur_hp, total_dmg, t_event });

            weaponNextT[activeW] = t_event + weaponInterval[activeW];

            if (cur_hp <= 0) {
                killed_at = shotNum;
                kill_time = t_event;
                break;
            }
            shotNum++;
        }

        // --- Вывод ---
        log("=== Цель ===", cHeader);
        log(QString("Пулестой (база):        %1").arg(base_hp, 10, 'f', 2));
        log(QString("Живка:                  +%1  (%2%)").arg(base_hp * mult, 10, 'f', 2).arg(mult * 100, 0, 'f', 2));
        log(QString("Полная приведа:         %1").arg(hp_full, 10, 'f', 2));
        log(QString("Отхилл:                 %1 HP/сек (%2%)").arg(hp_full * regen, 10, 'f', 2).arg(regen * 100, 0, 'f', 2));
        log(QString("Множитель урона:  голова × %1   корпус × %2").arg(head_mult).arg(body_mult));
        log("");

        log("=== Оружие ===", cHeader);
        log(QString("%1%2%3%4%5%6").arg("Название", -18).arg("урон", 9).arg("бронебой", 11).arg("уд/мин", 9).arg("интервал", 10).arg("× брони", 10));
        for (const auto& w : weapons) {
            double pm = pen_mult(w);
            double interval = 60.0 / w.aspd;
            log(QString("%1%2%3%4%5%6").arg(w.name, -18).arg(w.dmg, 9, 'f', 2).arg(QString::number(w.decr) + "%", 11).arg(w.aspd, 9, 'f', 1).arg(QString::number(interval) + "с", 10).arg(pm, 10, 'f', 4));
        }
        log("");

        log("=== Модули по оружию ===", cHeader);
        for (const auto& w : weapons) {
            log(QString("[%1]").arg(w.name));
            if (w.module_same) log(QString("  Модуль урона: × %1 (все зоны)").arg(w.module_mult));
            else log(QString("  Модуль урона: × %1 (голова) / × %2 (корпус)").arg(w.module_mult_head).arg(w.module_mult_body));
            if (w.exec_enabled) log(QString("  Добивание:    +%1% за 1% недост. HP").arg(w.exec_per_pct), cWarn);
            else log("  Добивание:    выключено", cDim);
        }
        log("");

        log("=== Симуляция (единый таймер на оружие) ===", cHeader);
        log("  #  группа зона мод  оружие           урон       HP после      суммарно     время", cDim);
        log(" " + QString("─").repeated(92), cDim);
        for (const auto& e : logData) {
            QString modS = e.mod ? "M" : "-";
            QString zoneS = (e.zone == "head") ? "H" : (e.zone == "body" ? "B" : "X");
            QString wname = e.w.name.left(14);
            QString gstr = QString("#%1").arg(e.gi + 1);

            QString line = QString(" %1  %2%3    %4   %5  %6  %7  %8")
                .arg(e.n, 3).arg(gstr, -6).arg(zoneS).arg(modS).arg(wname, -14).arg(e.dmg, 8, 'f', 2).arg(e.hp, 9, 'f', 2).arg(e.cum, 10, 'f', 2).arg(e.t, 7, 'f', 3) + "с";

            if (e.zone == "miss") log(line, cDim);
            else if (e.hp <= 0) log(line + " УБИТ", cGood);
            else log(line);
        }
        log("");

        log("=== ИТОГ ===", cHeader);
        log(QString("Всего выстрелов:        %1").arg(logData.size()));
        log(QString("Суммарный урон:         %1 HP").arg(total_dmg, 0, 'f', 2));
        log(QString("От HP цели:             %1%").arg((total_dmg / hp_full) * 100, 0, 'f', 2), cGood);
        log("");

        if (killed_at != -1) {
            log(QString("Цель убита на выстреле: #%1").arg(killed_at), cGood);
            log(QString("Время до убийства:      %1 сек").arg(kill_time, 0, 'f', 3), cGood);
            log(QString("Оверкилл:               %1 HP").arg(-cur_hp, 0, 'f', 2));
        }
        else {
            log(QString("Остаток HP:             %1").arg(std::max(cur_hp, 0.0), 0, 'f', 2), cWarn);
            log(QString("Недобито:               %1 HP  (%2% от HP цели)").arg(cur_hp, 0, 'f', 2).arg((cur_hp / hp_full) * 100, 0, 'f', 2), cBad);
        }
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Установка темной палитры для более приятного вида
    app.setStyle("Fusion");
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

    DamageCalculator window;
    window.show();
    return app.exec();
}

#include "main.moc"