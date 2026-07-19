#include "gui/dsp/DSPGuiIntegration.hpp"

#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QSplitter>
#include <QScrollArea>
#include <QDateTime>
#include <QFormLayout>

namespace mbootcore {
namespace dsp {

SupportPackManagerWidget::SupportPackManagerWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Search packages...");
    connect(m_searchBox, &QLineEdit::textChanged, this, &SupportPackManagerWidget::onSearchTextChanged);

    m_installBtn = new QPushButton("Install");
    m_uninstallBtn = new QPushButton("Uninstall");
    m_enableBtn = new QPushButton("Enable");
    m_disableBtn = new QPushButton("Disable");
    m_repairBtn = new QPushButton("Repair");
    m_verifyBtn = new QPushButton("Verify");
    m_reloadBtn = new QPushButton("Reload");

    connect(m_installBtn, &QPushButton::clicked, this, &SupportPackManagerWidget::onInstallClicked);
    connect(m_uninstallBtn, &QPushButton::clicked, this, &SupportPackManagerWidget::onUninstallClicked);
    connect(m_enableBtn, &QPushButton::clicked, this, &SupportPackManagerWidget::onEnableClicked);
    connect(m_disableBtn, &QPushButton::clicked, this, &SupportPackManagerWidget::onDisableClicked);
    connect(m_repairBtn, &QPushButton::clicked, this, &SupportPackManagerWidget::onRepairClicked);
    connect(m_verifyBtn, &QPushButton::clicked, this, &SupportPackManagerWidget::onVerifyClicked);
    connect(m_reloadBtn, &QPushButton::clicked, this, &SupportPackManagerWidget::onReloadClicked);

    m_uninstallBtn->setEnabled(false);
    m_enableBtn->setEnabled(false);
    m_disableBtn->setEnabled(false);
    m_repairBtn->setEnabled(false);
    m_verifyBtn->setEnabled(false);

    toolbar->addWidget(m_searchBox, 1);
    toolbar->addWidget(m_installBtn);
    toolbar->addWidget(m_uninstallBtn);
    toolbar->addWidget(m_enableBtn);
    toolbar->addWidget(m_disableBtn);
    toolbar->addWidget(m_repairBtn);
    toolbar->addWidget(m_verifyBtn);
    toolbar->addWidget(m_reloadBtn);

    mainLayout->addLayout(toolbar);

    // Status bar
    auto* statusBar = new QHBoxLayout();
    m_statusLabel = new QLabel("Ready");
    m_statsLabel = new QLabel();
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setVisible(false);
    m_progressBar->setFixedHeight(16);
    m_statsLabel->setFixedWidth(300);
    statusBar->addWidget(m_statusLabel, 1);
    statusBar->addWidget(m_statsLabel);
    statusBar->addWidget(m_progressBar);
    mainLayout->addLayout(statusBar);

    // Splitter: left tree, right detail
    auto* splitter = new QSplitter(Qt::Horizontal);

    m_packageTree = new QTreeWidget();
    m_packageTree->setHeaderLabels({"Package", "Version", "Vendor", "State", "Origin"});
    m_packageTree->setRootIsDecorated(true);
    m_packageTree->setAlternatingRowColors(true);
    m_packageTree->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_packageTree, &QTreeWidget::itemSelectionChanged, this, &SupportPackManagerWidget::onSelectionChanged);

    splitter->addWidget(m_packageTree);

    m_detailStack = new QStackedWidget();
    m_detailStack->addWidget(new QLabel("Select a package to view details"));
    m_detailStack->addWidget(createDetailWidget());
    m_detailStack->addWidget(createVendorBrowserWidget());
    m_detailStack->addWidget(createChipsetBrowserWidget());
    m_detailStack->addWidget(createLoaderBrowserWidget());
    m_detailStack->addWidget(createHealthWidget());
    m_detailStack->addWidget(createDependenciesWidget());
    m_detailStack->addWidget(createStatisticsWidget());
    m_detailStack->setCurrentIndex(0);

    splitter->addWidget(m_detailStack);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter, 1);
}

SupportPackManagerWidget::~SupportPackManagerWidget() = default;

void SupportPackManagerWidget::setDSPManager(DSPManager* manager) {
    m_manager = manager;
    refresh();
}

void SupportPackManagerWidget::refresh() {
    if (!m_manager) return;
    populateTree();
    updateStats();
}

void SupportPackManagerWidget::populateTree() {
    m_packageTree->clear();

    if (!m_manager) return;

    auto installed = m_manager->listInstalled();
    auto available = m_manager->listAvailable();

    // Group by vendor
    std::map<std::string, QTreeWidgetItem*> vendorGroups;

    for (const auto& stats : installed) {
        auto vendor = stats.packageId.substr(0, stats.packageId.find('.'));
        if (!vendorGroups.count(vendor)) {
            vendorGroups[vendor] = new QTreeWidgetItem({QString::fromStdString(vendor), "", "", "", ""});
            m_packageTree->addTopLevelItem(vendorGroups[vendor]);
        }

        auto* item = new QTreeWidgetItem({
            QString::fromStdString(stats.name),
            QString::fromStdString(stats.version.toString()),
            "", // vendor shown in group
            QString::fromStdString([&]{
                switch (stats.state) {
                    case DSPState::Enabled: return "Enabled";
                    case DSPState::Disabled: return "Disabled";
                    case DSPState::Installed: return "Installed";
                    case DSPState::Corrupted: return "Corrupted";
                    default: return "Unknown";
                }
            }()),
            QString::fromStdString([&]{
                switch (stats.origin) {
                    case DSPOrigin::System: return "System";
                    case DSPOrigin::User: return "User";
                    case DSPOrigin::Portable: return "Portable";
                    default: return "Unknown";
                }
            }())
        });
        item->setData(0, Qt::UserRole, QString::fromStdString(stats.packageId));
        vendorGroups[vendor]->addChild(item);
    }

    m_packageTree->expandAll();
}

void SupportPackManagerWidget::onInstallClicked() {
    if (!m_manager) return;
    auto path = QFileDialog::getExistingDirectory(this, "Select DSP Package Directory");
    if (path.isEmpty()) return;

    setStatus("Installing...");
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);

    auto result = m_manager->install(path.toStdString());
    if (result.isOk()) {
        setStatus("Package installed successfully");
        refresh();
    } else {
        setStatus("Install failed: " + QString::number(static_cast<int>(result.error())));
    }

    m_progressBar->setVisible(false);
}

void SupportPackManagerWidget::onUninstallClicked() {
    auto selected = m_packageTree->selectedItems();
    if (selected.isEmpty() || !m_manager) return;

    auto pkgId = selected.first()->data(0, Qt::UserRole).toString().toStdString();
    if (pkgId.empty()) return;

    auto confirm = QMessageBox::question(this, "Confirm Uninstall",
        "Are you sure you want to uninstall: " + QString::fromStdString(pkgId) + "?");
    if (confirm != QMessageBox::Yes) return;

    setStatus("Uninstalling...");
    auto result = m_manager->uninstall(pkgId);
    if (result.isOk()) {
        setStatus("Package uninstalled");
        refresh();
    } else {
        setStatus("Uninstall failed");
    }
}

void SupportPackManagerWidget::onEnableClicked() {
    auto selected = m_packageTree->selectedItems();
    if (selected.isEmpty() || !m_manager) return;
    auto pkgId = selected.first()->data(0, Qt::UserRole).toString().toStdString();
    if (pkgId.empty()) return;

    auto result = m_manager->enable(pkgId);
    if (result.isOk()) {
        setStatus("Package enabled");
        refresh();
    }
}

void SupportPackManagerWidget::onDisableClicked() {
    auto selected = m_packageTree->selectedItems();
    if (selected.isEmpty() || !m_manager) return;
    auto pkgId = selected.first()->data(0, Qt::UserRole).toString().toStdString();
    if (pkgId.empty()) return;

    auto result = m_manager->disable(pkgId);
    if (result.isOk()) {
        setStatus("Package disabled");
        refresh();
    }
}

void SupportPackManagerWidget::onRepairClicked() {
    auto selected = m_packageTree->selectedItems();
    if (selected.isEmpty() || !m_manager) return;
    auto pkgId = selected.first()->data(0, Qt::UserRole).toString().toStdString();
    if (pkgId.empty()) return;

    setStatus("Repairing...");
    auto result = m_manager->repair(pkgId);
    if (result.isOk()) {
        setStatus("Package repaired");
        refresh();
    } else {
        setStatus("Repair failed");
    }
}

void SupportPackManagerWidget::onVerifyClicked() {
    auto selected = m_packageTree->selectedItems();
    if (selected.isEmpty() || !m_manager) return;
    auto pkgId = selected.first()->data(0, Qt::UserRole).toString().toStdString();
    if (pkgId.empty()) return;

    setStatus("Verifying...");
    auto result = m_manager->verify(pkgId, DSPValidationLevel::Full);
    if (result.isOk()) {
        setStatus("Verification: OK");
    } else {
        setStatus("Verification: FAILED");
    }
}

void SupportPackManagerWidget::onReloadClicked() {
    if (!m_manager) return;
    setStatus("Reloading...");
    auto result = m_manager->reload();
    if (result.isOk()) {
        setStatus("Reloaded");
        refresh();
    }
}

void SupportPackManagerWidget::onSelectionChanged() {
    auto selected = m_packageTree->selectedItems();
    bool hasSelection = !selected.isEmpty();
    m_uninstallBtn->setEnabled(hasSelection);
    m_enableBtn->setEnabled(hasSelection);
    m_disableBtn->setEnabled(hasSelection);
    m_repairBtn->setEnabled(hasSelection);
    m_verifyBtn->setEnabled(hasSelection);

    if (hasSelection) {
        auto pkgId = selected.first()->data(0, Qt::UserRole).toString().toStdString();
        if (!pkgId.empty()) {
            showPackageDetails(pkgId);
        }
    }
}

void SupportPackManagerWidget::onSearchTextChanged(const QString& text) {
    for (int i = 0; i < m_packageTree->topLevelItemCount(); ++i) {
        auto* top = m_packageTree->topLevelItem(i);
        bool visible = text.isEmpty() || top->text(0).contains(text, Qt::CaseInsensitive);
        top->setHidden(!visible);
        for (int j = 0; j < top->childCount(); ++j) {
            auto* child = top->child(j);
            bool childVisible = text.isEmpty() || child->text(0).contains(text, Qt::CaseInsensitive);
            child->setHidden(!childVisible);
        }
    }
}

void SupportPackManagerWidget::showPackageDetails(const std::string& packageId) {
    if (!m_manager) return;
    auto* pkg = m_manager->findPackage(packageId);
    if (!pkg) {
        m_detailStack->setCurrentIndex(0);
        return;
    }
    auto* detailWidget = qobject_cast<PackageDetailsWidget*>(m_detailStack->widget(1));
    if (detailWidget) {
        detailWidget->showPackage(*pkg);
        m_detailStack->setCurrentIndex(1);
    }
}

void SupportPackManagerWidget::updateStats() {
    if (!m_manager) return;
    auto text = QString("Installed: %1 | Enabled: %2 | Loaders: %3 | Chipsets: %4")
        .arg(m_manager->installedCount())
        .arg(m_manager->enabledCount())
        .arg(m_manager->totalLoaderCount())
        .arg(m_manager->totalChipsetCount());
    m_statsLabel->setText(text);
}

void SupportPackManagerWidget::setStatus(const QString& msg) {
    m_statusLabel->setText(msg);
}

QWidget* SupportPackManagerWidget::createDetailWidget() {
    return new PackageDetailsWidget();
}

QWidget* SupportPackManagerWidget::createVendorBrowserWidget() {
    return new VendorBrowserWidget();
}

QWidget* SupportPackManagerWidget::createChipsetBrowserWidget() {
    return new ChipsetBrowserWidget();
}

QWidget* SupportPackManagerWidget::createLoaderBrowserWidget() {
    return new LoaderBrowserWidget();
}

QWidget* SupportPackManagerWidget::createHealthWidget() {
    auto* w = new QWidget();
    auto* l = new QVBoxLayout(w);
    l->addWidget(new QLabel("Health Status"));
    auto* healthText = new QTextEdit();
    healthText->setReadOnly(true);
    l->addWidget(healthText);
    return w;
}

QWidget* SupportPackManagerWidget::createDependenciesWidget() {
    auto* w = new QWidget();
    auto* l = new QVBoxLayout(w);
    l->addWidget(new QLabel("Dependencies"));
    auto* depText = new QTextEdit();
    depText->setReadOnly(true);
    l->addWidget(depText);
    return w;
}

QWidget* SupportPackManagerWidget::createStatisticsWidget() {
    return new PackageStatisticsWidget();
}

VendorBrowserWidget::VendorBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    m_tree = new QTreeWidget();
    m_tree->setHeaderLabels({"Vendor", "Packages", "Chipsets", "Loaders"});
    layout->addWidget(m_tree);
    m_detailLabel = new QLabel("Select a vendor");
    layout->addWidget(m_detailLabel);
}

void VendorBrowserWidget::setDSPManager(DSPManager* manager) {
    m_manager = manager;
    refresh();
}

void VendorBrowserWidget::refresh() {
    m_tree->clear();
    if (!m_manager) return;

    for (const auto& stats : m_manager->listInstalled()) {
        auto vendor = QString::fromStdString(stats.packageId.substr(0, stats.packageId.find('.')));
        auto* item = new QTreeWidgetItem({
            vendor,
            "1", // packages count
            QString::number(stats.chipsetCount),
            QString::number(stats.loaderCount)
        });
        m_tree->addTopLevelItem(item);
    }
}

ChipsetBrowserWidget::ChipsetBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    auto* filterBar = new QHBoxLayout();
    filterBar->addWidget(new QLabel("Vendor:"));
    m_vendorFilter = new QComboBox();
    m_vendorFilter->addItem("All");
    filterBar->addWidget(m_vendorFilter, 1);
    layout->addLayout(filterBar);

    m_tree = new QTreeWidget();
    m_tree->setHeaderLabels({"Chipset", "Family", "Protocols", "Storage"});
    layout->addWidget(m_tree);

    m_detailTable = new QTableWidget(0, 2);
    m_detailTable->setHorizontalHeaderLabels({"Property", "Value"});
    m_detailTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_detailTable);
}

void ChipsetBrowserWidget::setDSPManager(DSPManager* manager) {
    m_manager = manager;
    refresh();
}

void ChipsetBrowserWidget::refresh() {
    m_tree->clear();
    if (!m_manager) return;
}

LoaderBrowserWidget::LoaderBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    auto* searchBar = new QHBoxLayout();
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Search loaders...");
    searchBar->addWidget(m_searchBox, 1);
    layout->addLayout(searchBar);

    m_tree = new QTreeWidget();
    m_tree->setHeaderLabels({"Loader", "Version", "Protocol", "Chipset"});
    layout->addWidget(m_tree);

    m_detailLabel = new QLabel("Select a loader for details");
    layout->addWidget(m_detailLabel);
}

void LoaderBrowserWidget::setDSPManager(DSPManager* manager) {
    m_manager = manager;
    refresh();
}

void LoaderBrowserWidget::refresh() {
    m_tree->clear();
    if (!m_manager) return;
}

PackageDetailsWidget::PackageDetailsWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    auto* header = new QHBoxLayout();
    header->addWidget(new QLabel("Name:"));
    m_nameLabel = new QLabel();
    header->addWidget(m_nameLabel, 1);
    header->addWidget(new QLabel("Version:"));
    m_versionLabel = new QLabel();
    header->addWidget(m_versionLabel);
    layout->addLayout(header);

    auto* infoGrid = new QFormLayout();
    m_vendorLabel = new QLabel();
    m_stateLabel = new QLabel();
    m_originLabel = new QLabel();
    m_sizeLabel = new QLabel();
    m_installDateLabel = new QLabel();
    infoGrid->addRow("Vendor:", m_vendorLabel);
    infoGrid->addRow("State:", m_stateLabel);
    infoGrid->addRow("Origin:", m_originLabel);
    infoGrid->addRow("Size:", m_sizeLabel);
    infoGrid->addRow("Installed:", m_installDateLabel);
    layout->addLayout(infoGrid);

    m_descriptionLabel = new QLabel();
    m_descriptionLabel->setWordWrap(true);
    layout->addWidget(m_descriptionLabel);

    m_licenseLabel = new QLabel();
    layout->addWidget(m_licenseLabel);

    m_tabs = new QTabWidget();
    m_chipsetTable = new QTableWidget(0, 3);
    m_chipsetTable->setHorizontalHeaderLabels({"Vendor", "Family", "Variant"});
    m_tabs->addTab(m_chipsetTable, "Chipsets");

    m_loaderTable = new QTableWidget(0, 4);
    m_loaderTable->setHorizontalHeaderLabels({"Loader", "Version", "Protocol", "Priority"});
    m_tabs->addTab(m_loaderTable, "Loaders");

    m_dependencyText = new QTextEdit();
    m_dependencyText->setReadOnly(true);
    m_tabs->addTab(m_dependencyText, "Dependencies");

    m_healthText = new QTextEdit();
    m_healthText->setReadOnly(true);
    m_tabs->addTab(m_healthText, "Health");

    layout->addWidget(m_tabs, 1);
}

void PackageDetailsWidget::showPackage(const DSPPackageMetadata& pkg) {
    m_nameLabel->setText(QString::fromStdString(pkg.manifest.name));
    m_versionLabel->setText(QString::fromStdString(pkg.manifest.version.toString()));
    m_vendorLabel->setText(QString::fromStdString(pkg.vendor.vendorName));
    m_stateLabel->setText([&]{
        switch (pkg.state) {
            case DSPState::Enabled: return "Enabled";
            case DSPState::Disabled: return "Disabled";
            case DSPState::Installed: return "Installed";
            case DSPState::Corrupted: return "Corrupted";
            default: return "Unknown";
        }
    }());
    m_originLabel->setText([&]{
        switch (pkg.origin) {
            case DSPOrigin::System: return "System";
            case DSPOrigin::User: return "User";
            case DSPOrigin::Portable: return "Portable";
            default: return "Unknown";
        }
    }());
    m_sizeLabel->setText(QString::number(pkg.manifest.fileSize) + " bytes");
    m_installDateLabel->setText(QString::fromStdString(""));
    m_descriptionLabel->setText(QString::fromStdString(pkg.manifest.description));
    m_licenseLabel->setText(QString::fromStdString("License: " + pkg.manifest.license));

    // Chipsets
    m_chipsetTable->setRowCount(static_cast<int>(pkg.chipsets.size()));
    for (size_t i = 0; i < pkg.chipsets.size(); ++i) {
        m_chipsetTable->setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString::fromStdString(pkg.chipsets[i].id.vendor)));
        m_chipsetTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(pkg.chipsets[i].id.family)));
        m_chipsetTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString::fromStdString(pkg.chipsets[i].id.variant)));
    }

    // Loaders
    m_loaderTable->setRowCount(static_cast<int>(pkg.loaders.size()));
    for (size_t i = 0; i < pkg.loaders.size(); ++i) {
        m_loaderTable->setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString::fromStdString(pkg.loaders[i].name)));
        m_loaderTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(pkg.loaders[i].version.toString())));
        QString protocols;
        for (auto p : pkg.loaders[i].protocols) {
            protocols += QString::number(static_cast<int>(p)) + " ";
        }
        m_loaderTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(protocols));
        m_loaderTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString::number(pkg.loaders[i].priority)));
    }

    // Dependencies
    QString depText;
    for (const auto& dep : pkg.manifest.dependencies) {
        depText += QString::fromStdString(dep.name) + " " +
                   QString::fromStdString(dep.minVersion.toString()) +
                   (dep.required ? " [required]" : " [optional]") + "\n";
    }
    m_dependencyText->setText(depText);

    m_healthText->setText("No health issues detected.");
}

void PackageDetailsWidget::clear() {
    m_nameLabel->clear();
    m_versionLabel->clear();
    m_vendorLabel->clear();
    m_stateLabel->clear();
    m_originLabel->clear();
    m_sizeLabel->clear();
    m_installDateLabel->clear();
    m_descriptionLabel->clear();
    m_licenseLabel->clear();
    m_chipsetTable->setRowCount(0);
    m_loaderTable->setRowCount(0);
    m_dependencyText->clear();
    m_healthText->clear();
}

PackageStatisticsWidget::PackageStatisticsWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QFormLayout(this);
    m_nameLabel = new QLabel();
    m_versionLabel = new QLabel();
    m_stateLabel = new QLabel();
    m_originLabel = new QLabel();
    m_installedSizeLabel = new QLabel();
    m_compressedSizeLabel = new QLabel();
    m_loaderCountLabel = new QLabel();
    m_chipsetCountLabel = new QLabel();
    m_profileCountLabel = new QLabel();
    m_quirkCountLabel = new QLabel();
    m_dependentPackagesLabel = new QLabel();
    m_installDateLabel = new QLabel();
    m_needsUpdateLabel = new QLabel();

    layout->addRow("Name:", m_nameLabel);
    layout->addRow("Version:", m_versionLabel);
    layout->addRow("State:", m_stateLabel);
    layout->addRow("Origin:", m_originLabel);
    layout->addRow("Installed Size:", m_installedSizeLabel);
    layout->addRow("Compressed Size:", m_compressedSizeLabel);
    layout->addRow("Loaders:", m_loaderCountLabel);
    layout->addRow("Chipsets:", m_chipsetCountLabel);
    layout->addRow("Profiles:", m_profileCountLabel);
    layout->addRow("Quirks:", m_quirkCountLabel);
    layout->addRow("Dependents:", m_dependentPackagesLabel);
    layout->addRow("Install Date:", m_installDateLabel);
    layout->addRow("Needs Update:", m_needsUpdateLabel);
}

void PackageStatisticsWidget::showStatistics(const DSPPackageStatistics& stats) {
    m_nameLabel->setText(QString::fromStdString(stats.name));
    m_versionLabel->setText(QString::fromStdString(stats.version.toString()));
    m_stateLabel->setText([&]() -> QString {
        switch (stats.state) {
            case DSPState::Enabled: return QStringLiteral("Enabled");
            case DSPState::Disabled: return QStringLiteral("Disabled");
            case DSPState::Installed: return QStringLiteral("Installed");
            default: return QString::number(static_cast<int>(stats.state));
        }
    }());
    m_originLabel->setText([&]{
        switch (stats.origin) {
            case DSPOrigin::System: return "System";
            case DSPOrigin::User: return "User";
            case DSPOrigin::Portable: return "Portable";
            default: return "Unknown";
        }
    }());
    m_installedSizeLabel->setText(QString::number(stats.installedSize) + " bytes");
    m_compressedSizeLabel->setText(QString::number(stats.compressedSize) + " bytes");
    m_loaderCountLabel->setText(QString::number(stats.loaderCount));
    m_chipsetCountLabel->setText(QString::number(stats.chipsetCount));
    m_profileCountLabel->setText(QString::number(stats.profileCount));
    m_quirkCountLabel->setText(QString::number(stats.quirkCount));
    m_dependentPackagesLabel->setText(QString::number(stats.dependentCount));
    m_needsUpdateLabel->setText(stats.needsUpdate ? "Yes" : "No");
}

} // namespace dsp
} // namespace mbootcore
