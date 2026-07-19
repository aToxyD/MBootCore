#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QGroupBox>
#include <QTextEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QProgressBar>
#include <QTabWidget>

#include <string>
#include <vector>
#include <memory>

#include <mbootcore/dsp/DSPTypes.hpp>
#include <mbootcore/dsp/DSPMetadata.hpp>
#include <mbootcore/dsp/DSPManager.hpp>
#include <mbootcore/dsp/DSPInspector.hpp>
#include <mbootcore/dsp/DSPValidator.hpp>

namespace mbootcore {
namespace dsp {

class SupportPackManagerWidget : public QWidget {
    Q_OBJECT
public:
    explicit SupportPackManagerWidget(QWidget* parent = nullptr);
    ~SupportPackManagerWidget() override;

    void setDSPManager(DSPManager* manager);
    void refresh();

private slots:
    void onInstallClicked();
    void onUninstallClicked();
    void onEnableClicked();
    void onDisableClicked();
    void onRepairClicked();
    void onVerifyClicked();
    void onReloadClicked();
    void onSelectionChanged();
    void onSearchTextChanged(const QString& text);

private:
    DSPManager* m_manager{nullptr};

    QLineEdit* m_searchBox{nullptr};
    QTreeWidget* m_packageTree{nullptr};
    QStackedWidget* m_detailStack{nullptr};
    QPushButton* m_installBtn{nullptr};
    QPushButton* m_uninstallBtn{nullptr};
    QPushButton* m_enableBtn{nullptr};
    QPushButton* m_disableBtn{nullptr};
    QPushButton* m_repairBtn{nullptr};
    QPushButton* m_verifyBtn{nullptr};
    QPushButton* m_reloadBtn{nullptr};
    QLabel* m_statusLabel{nullptr};
    QLabel* m_statsLabel{nullptr};
    QProgressBar* m_progressBar{nullptr};

    QWidget* createDetailWidget();
    QWidget* createVendorBrowserWidget();
    QWidget* createChipsetBrowserWidget();
    QWidget* createLoaderBrowserWidget();
    QWidget* createHealthWidget();
    QWidget* createDependenciesWidget();
    QWidget* createStatisticsWidget();

    void populateTree();
    void showPackageDetails(const std::string& packageId);
    void updateStats();
    void setStatus(const QString& msg);
};

class VendorBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit VendorBrowserWidget(QWidget* parent = nullptr);
    void setDSPManager(DSPManager* manager);
    void refresh();
private:
    DSPManager* m_manager{nullptr};
    QTreeWidget* m_tree{nullptr};
    QLabel* m_detailLabel{nullptr};
};

class ChipsetBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChipsetBrowserWidget(QWidget* parent = nullptr);
    void setDSPManager(DSPManager* manager);
    void refresh();
private:
    DSPManager* m_manager{nullptr};
    QTreeWidget* m_tree{nullptr};
    QTableWidget* m_detailTable{nullptr};
    QComboBox* m_vendorFilter{nullptr};
};

class LoaderBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit LoaderBrowserWidget(QWidget* parent = nullptr);
    void setDSPManager(DSPManager* manager);
    void refresh();
private:
    DSPManager* m_manager{nullptr};
    QTreeWidget* m_tree{nullptr};
    QLabel* m_detailLabel{nullptr};
    QLineEdit* m_searchBox{nullptr};
};

class PackageDetailsWidget : public QWidget {
    Q_OBJECT
public:
    explicit PackageDetailsWidget(QWidget* parent = nullptr);
    void showPackage(const DSPPackageMetadata& pkg);
    void clear();
private:
    QLabel* m_nameLabel{nullptr};
    QLabel* m_versionLabel{nullptr};
    QLabel* m_vendorLabel{nullptr};
    QLabel* m_stateLabel{nullptr};
    QLabel* m_originLabel{nullptr};
    QLabel* m_sizeLabel{nullptr};
    QLabel* m_installDateLabel{nullptr};
    QLabel* m_descriptionLabel{nullptr};
    QLabel* m_licenseLabel{nullptr};
    QTabWidget* m_tabs{nullptr};
    QTableWidget* m_chipsetTable{nullptr};
    QTableWidget* m_loaderTable{nullptr};
    QTextEdit* m_dependencyText{nullptr};
    QTextEdit* m_healthText{nullptr};
};

class PackageStatisticsWidget : public QWidget {
    Q_OBJECT
public:
    explicit PackageStatisticsWidget(QWidget* parent = nullptr);
    void showStatistics(const DSPPackageStatistics& stats);
private:
    QLabel* m_nameLabel{nullptr};
    QLabel* m_versionLabel{nullptr};
    QLabel* m_stateLabel{nullptr};
    QLabel* m_originLabel{nullptr};
    QLabel* m_installedSizeLabel{nullptr};
    QLabel* m_compressedSizeLabel{nullptr};
    QLabel* m_loaderCountLabel{nullptr};
    QLabel* m_chipsetCountLabel{nullptr};
    QLabel* m_profileCountLabel{nullptr};
    QLabel* m_quirkCountLabel{nullptr};
    QLabel* m_dependentPackagesLabel{nullptr};
    QLabel* m_installDateLabel{nullptr};
    QLabel* m_needsUpdateLabel{nullptr};
};

} // namespace dsp
} // namespace mbootcore
