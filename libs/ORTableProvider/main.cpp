/**
 * @brief This file is destined to be used in the sdcX training course hands-on parts. It models an operating table, which MDIB is located in
 * resources/ORTableMDIB.xml. The demo code manages a virtual OR table that stores the value per axis.
 * The MDIB consists of two channels, one for orientation (trend, tilt, backplate) and table height, one for predefined positions. 
 * It contains one alert system with one alert condition per axis that shall be triggered, when the axis are in a delta of 5° to the respective maximum
 * For operations, the table offers activates for each axis movement and SetContextState operations for setting Workflow and Patient Contexts. 
 * Also, for alert signal handling, a SetAlertStateOperation is added.
 *
 * @copyright 2023 SurgiTAIX AG
 *
 */

#include "SDCCore/Core.h"
#include "SDCCore/Prerequisites.h"
#include "ProviderAPI/SDCProvider.h"
#include "ProviderAPI/MDIBAccess/ProviderMDIBAccess.h"
#include "ProviderAPI/ProviderCore/SetLayer/ProviderSetHandler.h"

#include "Common/StringHelper.h"
#include "Common/DateTimeHelper.h"

#include "Logging/LogBroker.h"
#include "Logging/Loggers/ConsoleLogger.h"
#include "Logging/Loggers/FileLogger.h"

#include "MessageModel/MSG/Activate.h"

#include "ProviderAPI/StateHandler/ExternalControlHandler.h"
#include "ProviderAPI/StateHandler/TransactionHandler.h"
#include "ProviderAPI/StateHandler/SetOperationStatesContainer/SetValueStates.h"
#include "ProviderAPI/StateHandler/SetOperationStatesContainer/SetStringStates.h"

#include "ProviderAPI/StateHandler/SetOperationStatesContainer/ActivateStates.h"
#include "ProviderAPI/StateHandler/DenyAllExternalControlHandler.h"

#include "ParticipantModel/PM/StringMetricState.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <thread>

// Change this to a unique EPR
const std::string PROVIDER_EPR("TODO");

// In case the provider shall be started without TLS, set this variable to false
constexpr bool ENABLE_TLS{true};

// Using definitions for increased readability 
using namespace Logging;
using namespace ProviderAPI;
using namespace ProviderAPI::StateHandler;
using namespace std::chrono_literals;

enum class PredefinedPosition
{
    NullLevel,
    BeachChair
};

struct VirtualORTable
{
    double height = 80; // 60-140cm
    double trend = 39.8; // -45° till +45°
    double tilt; // -25° till +25°
    double backplate; // -40° till +80°
    PredefinedPosition predefinedPosition;
};

// Global object for data access
VirtualORTable virtualTable;

/**
 * @brief This state handler is used for Activate requests. On each Activate request for an enabled operation,
 * the "onNewTransaction"-method is triggered. Here, the Activate is logged and the next mode is selected.
 */
class ORTableSetStringHandler : public ExternalControlHandler<SetOperationStatesContainer::SetStringStates>
{
private:
public:
    ORTableSetStringHandler() = default;

    // call to user code
    virtual void
        onNewTransaction(std::shared_ptr<TransactionHandler<SetOperationStatesContainer::SetStringStates>> p_transactionHandler) override
    {
        /* 
        
        TODO 
        When receiving the MDC_OR_TABLE_SETSTRING_PREDEFINED_POSITIONS_SCO operation, set the virtualTable.predefinedPosition accordingly. Then, transition to "FIN" 
        */
    }
};


/**
 * @brief This state handler is used for Activate requests. On each Activate request for an enabled operation, 
 * the "onNewTransaction"-method is triggered. Here, the Activate is logged and the next mode is selected.
 */
class ORTableActivateHandler : public ExternalControlHandler<SetOperationStatesContainer::ActivateStates>
{
private:
public:
    ORTableActivateHandler() = default;

    // call to user code
    virtual void 
        onNewTransaction(std::shared_ptr<TransactionHandler<SetOperationStatesContainer::ActivateStates>> p_transactionHandler) override
    {
        /*
            TODO
            Upon receiving an activate on MDC_OR_TABLE_ACTIVATE_APPLY_PREDEFINED_POSITION, apply the predefined position in the virtual table
            Nulllevel: Height 80, Rest 0
            Beach Chair: Height 80, Trend 0, Tilt 0, Backplate 45

            Upon receiving any other activate, increase/decrease the value by 1 (height), 0.1 (angles). Keep the margins in mind. 
            Then, traverse directly to FIN
        */
    }
};

/**
 * @brief This state handler is used for SetContextState requests. On each SetContextState request for an enabled operation,
 * the "onNewTransaction"-method is triggered. Here, the SetContextState is logged and the next mode is selected.
 */
class ORTableSetContextStateHandler : public ExternalControlHandler<SetOperationStatesContainer::SetContextStates>
{
private:
public:
    ORTableSetContextStateHandler() = default;

    // call to user code
    virtual void
        onNewTransaction(std::shared_ptr<TransactionHandler<SetOperationStatesContainer::SetContextStates>> p_transactionHandler) override
    {

        p_transactionHandler->transitionFromEntryTo(UserInterfaces::Set::OnEntryInvocationState::Wait);
        p_transactionHandler->transitionFromWaitingTo(UserInterfaces::Set::OnWaitInvocationState::Start);
        p_transactionHandler->transitionFromStartedTo(UserInterfaces::Set::OnStartedInvocationState::Fin);
    }
};


/**
 * @brief This state handler is used for SetAlert requests. On each SetAlert request for an enabled operation,
 * the "onNewTransaction"-method is triggered. Here, the SetAlert is logged and the next mode is selected.
 */
class ORTableSetAlertStateHandler : public ExternalControlHandler<SetOperationStatesContainer::SetAlertStates>
{
private:
public:
    ORTableSetAlertStateHandler() = default;

    // call to user code
    virtual void
        onNewTransaction(std::shared_ptr<TransactionHandler<SetOperationStatesContainer::SetAlertStates>> p_transactionHandler) override
    {

        p_transactionHandler->transitionFromEntryTo(UserInterfaces::Set::OnEntryInvocationState::Wait);
        p_transactionHandler->transitionFromWaitingTo(UserInterfaces::Set::OnWaitInvocationState::Start);
        p_transactionHandler->transitionFromStartedTo(UserInterfaces::Set::OnStartedInvocationState::Fin);
    }
};


// The model and device description data are send to a consumer answering Get Request (DPWS). 
// They contain general information of the model, such as the model and manucaturers name and 
// the device such as the serial number or firmware version
std::shared_ptr<MessageModel::DPWS::ThisModel> prepareModelDescription()
{
    using namespace MessageModel::DPWS;

    std::vector<ThisModel::Manufacturer> manufacturerNames;
    manufacturerNames.push_back(ThisModel::Manufacturer{{"SurgiTAIX"}});

    std::vector<ThisModel::ModelName> modelNames;
    modelNames.push_back(ThisModel::ModelName{{"sdcX OR Table Demo Provider"}});

    auto model = std::make_shared<ThisModel>(std::move(manufacturerNames), std::move(modelNames));
    model->setModelUrl(XS::AnyURI("http://surgitaix.com"));
    model->setPresentationUrl(ThisModel::PresentationUrl{"http://surgitaix.com"});
    model->setManufacturerUrl(ThisModel::ManufacturerUrl{"http://surgitaix.com"});
    model->setModelNumber(ThisModel::ModelNumber{"1234"});

    return model;
}
std::shared_ptr<MessageModel::DPWS::ThisDevice> prepareDeviceDescription()
{
    using namespace MessageModel::DPWS;

    std::vector<ThisDevice::FriendlyName> friendlyNames;
    friendlyNames.emplace_back(std::string{"sdcX OR Table Demo Provider"});

    auto device = std::make_shared<ThisDevice>(std::move(friendlyNames));
    device->setSerialNumber(ThisDevice::SerialNumber("4567"));
    device->setFirmwareVersion(ThisDevice::FirmwareVersion{"1.3.0"});
    return device;
}

// generates config that contains the locations of the TLS-certificates
std::shared_ptr<Config::TLSConfig> createTLSConfig()
{
    auto tlsConfig = std::make_shared<Config::TLSConfig>();

    tlsConfig->setTrustedAuthorityLocation("./certificates/pat_ca.pem");
    tlsConfig->setCertificateLocation("./certificates/pat_cert.pem");
    tlsConfig->setPrivateKeyLocation("./certificates/pat_private.pem");

    return tlsConfig;
}

// The Provider config contains all information needed to set up a provider. The configs content is described in the regarding class.
std::shared_ptr<Config::ProviderConfig> createProviderConfig(std::shared_ptr<SDCCommon::DataTypes::NetworkInterface> p_networkInterface,
                                                             const SDCCommon::DataTypes::NetworkAddress p_localAddress)
{
    auto config = std::make_shared<Config::ProviderConfig>(PROVIDER_EPR,
                                                           (ENABLE_TLS ? createTLSConfig() : nullptr),
                                                           prepareModelDescription(),
                                                           prepareDeviceDescription(),
                                                           p_localAddress,
                                                           p_networkInterface);

    return config;
}


// This class runs a task that updates the tables position values. 
// Think of this as the RS232 connection that is regularly updated
// In this case, the virtual table model is moved into SDC description
class ValueUpdater
{
private:
    ProviderAPI::SDCProvider* m_provider{nullptr};

    std::atomic<bool> m_running{true};
    std::thread m_thread;

public:
    
    ValueUpdater(ProviderAPI::SDCProvider* p_provider)
        : m_provider(p_provider)
    {
    }
    ~ValueUpdater()
    {
        if(m_running)
        {
            stop();
        }
    }

    void applyChanges()
    {
        auto time = DateTimeHelper::millisecondsSinceEpoch();


        // Update changes 
        auto updateAccess = m_provider->getMDIBGateway()->makeUpdateAccess();

        /*   
            TODO 
            Update the numeric metric values using the given update access
        */

        auto result = m_provider->getMDIBGateway()->commit(std::move(updateAccess));
        if (!result.success())
        {
            std::cout << "Update of values not successful: " + result.getError();
        }
    }

    void applyAlarms()
    {
        auto time = DateTimeHelper::millisecondsSinceEpoch();

        // Update changes 
        auto updateAccess = m_provider->getMDIBGateway()->makeUpdateAccess();
        
        /*
            TODO
            Check the margins and trigger the alert conditions and signals
            In case no alert is present, deactivate them
        
        */

        // Height
        if (virtualTable.height >= 135 && virtualTable.height <= 140)
        {


        }
        else if (virtualTable.height <= 65 && virtualTable.height >= 60)
        {

        }
        else
        {
        }
        // Trend
        if (virtualTable.trend >= 40 && virtualTable.trend <= 45)
        {

        }
        else if (virtualTable.trend <= -40 && virtualTable.trend >= -45)
        {

        }
        else
        {
        }
        // Tilt
        if (virtualTable.tilt >= 20 && virtualTable.tilt <= 25)
        {

        }
        else if (virtualTable.tilt <= -20 && virtualTable.tilt >= -25)
        {

        }
        else
        {
        }
        // Back
        if (virtualTable.backplate >= 75 && virtualTable.backplate <= 80)
        {

        }
        else if (virtualTable.backplate <= -35 && virtualTable.backplate >= -40)
        {

        }
        else
        {
        }

        auto result = m_provider->getMDIBGateway()->commit(std::move(updateAccess));
        if (!result.success())
        {
            std::cout << "Update of values not successful: " + result.getError();
        }
    }

    void stop()
    {
        m_running = false;
        m_thread.join();
    }

    void run()
    {
        m_thread = std::thread([&]() {
            while(m_running)
            {
                applyChanges();
                applyAlarms();

                
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
    }
};

int main()
{
    /*
    * 
    * SETUP 
    * 
    */

    // Logger setup: A logger to console and one file logger are set up
    std::string consoleLoggerTag = "console";
    std::string fileLoggerTag = "file";
    
    auto consoleLogger = std::make_shared<Loggers::ConsoleLogger>();
    auto fileLogger = std::make_shared<Loggers::FileLogger>("ORTable.log");

    LogBroker::getInstance().registerLogger(consoleLoggerTag, consoleLogger);
    LogBroker::getInstance().registerLogger(fileLoggerTag, fileLogger);
    
    // Log Level is managed centrally 
    LogBroker::getInstance().setLogLevel(Severity::Notice);

    
    // 
    // Configuration of network: select local address / interface

    // Adapt as needed
    const std::string IP{""};
    const unsigned int PORT{10000};

    std::shared_ptr<SDCCommon::DataTypes::NetworkInterface> networkInterface{nullptr};
    std::shared_ptr<SDCCommon::DataTypes::NetworkAddress> localAddress{nullptr};

    if(SDCCore::getNetworkInterfaceByIPAddress(IP))
    {
        networkInterface = std::make_shared<SDCCommon::DataTypes::NetworkInterface>(
            *SDCCore::getNetworkInterfaceByIPAddress(IP));
        localAddress = std::make_shared<SDCCommon::DataTypes::NetworkAddress>(networkInterface->getIPv4Addresses()[0], PORT);
    }
    else
    {
        LogBroker::getInstance().log(
            LogMessage("ORTableProvider", Severity::Notice, "Could not find adapter with IP: " + IP));
        LogBroker::getInstance().log(LogMessage("ORTableProvider", Severity::Notice, "Binding to default instead"));
        networkInterface = std::make_shared<SDCCommon::DataTypes::NetworkInterface>(SDCCore::enumerateNetworkInterfaces()[0]);
        localAddress = std::make_shared<SDCCommon::DataTypes::NetworkAddress>(networkInterface->getIPv4Addresses()[0], PORT);
        LogBroker::getInstance().log(
            LogMessage("ORTableProvider", Severity::Notice, "Selected default Adapter: " + localAddress->toString()));
    }

    // setting up the core of the sdcX stack
    auto coreConfig = std::make_unique<Config::CoreConfig>();
    auto sdcCore = SDCCore::Core::createInstance(std::move(coreConfig));
    LogBroker::getInstance().log(LogMessage("ORTableProvider", Severity::Notice, "Core created!"));

    // setting up the Provider
    auto providerConfig{createProviderConfig(networkInterface, *localAddress)};
    // default config. Local address to bind to must be specified
    auto discoveryConfig = std::make_shared<Config::DiscoveryConfig>(localAddress->getIPAddress());  
    discoveryConfig->setDiscoverySendingEndpointPort(5011);

    LogBroker::getInstance().log(
        LogMessage("ORTableProvider", Severity::Notice, "Binding to " + localAddress->getIPAddress().getAddress()));

    auto provider = std::make_unique<ProviderAPI::SDCProvider>(sdcCore, providerConfig, discoveryConfig);

    LogBroker::getInstance().log({"ORTableProvider", Severity::Notice, "Provider created!"});
    
    // Load MDIB from xml file (contained in <MdibResponse> element)
    const auto mdibData = Common::StringHelper::loadFile("ORTableMDIB.xml");
    try
    {
        if(provider.get()->loadMdib(mdibData) == false)
        {
            LogBroker::getInstance().log({"ORTableProvider", Severity::Notice, "Could not load Mdib!"});
        }
        else
        {
            LogBroker::getInstance().log({"ORTableProvider", Severity::Notice, "Successfully loaded Mdib!"});
        }
    }
    catch(const ProviderAPI::ProviderAPIException& e)
    {
        std::cout << "Failed to load Mdib: " << e.what() << std::endl;
        return -1;
    }


    //
    // Create Handlers to listen for events as needed
    auto setStringHandler = std::make_shared<ORTableSetStringHandler>();
    auto orTableActivateHandler = std::make_shared<ORTableActivateHandler>();
    auto orTableSetAlertStateHandler = std::make_shared<ORTableSetAlertStateHandler>();
    auto orTableSetContextStateHandler = std::make_shared<ORTableSetContextStateHandler>();


    provider->registerSetStringExternalControlHandler(setStringHandler);
    provider->registerActivateExternalControlHandler(orTableActivateHandler);
    provider->registerSetAlertStateExternalControlHandler(orTableSetAlertStateHandler);
    provider->registerSetContextStateExternalControlHandler(orTableSetContextStateHandler);

    /*
    * 
    * RUNTIME
    * 
    */
    // after setting up everything, the provider can be started
    // TODO

    // start a thread that simulates an update of the values and notifies all connected consumers
    auto valueUpdater = std::make_unique<ValueUpdater>(provider.get());
    valueUpdater->run();


    // Stop condition
    std::cout << "Press key to exit: ";
    std::cin.get();


    // Cleanup 
    valueUpdater->stop();
    provider.reset();
    sdcCore.reset();

    // Stop condition
    std::cout << "Press key to really exit: ";
    std::cin.get();

    LogBroker::getInstance().unregisterLogger(consoleLoggerTag);  
    LogBroker::getInstance().unregisterLogger(fileLoggerTag);


    return 0;
}
