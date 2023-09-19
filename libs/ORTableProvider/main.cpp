/**
 * @brief This file is destined to be used in the sdcX training course hands-on parts. It models an operating table, which MDIB is located in
 * resources/ORTableMDIB.xml
 * The MDIB consists of two channels, one for orientation (trend, tilt, backplate) and table height, one for battery information. 
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
#include "ProviderAPI/StateHandler/SetOperationStatesContainer/ActivateStates.h"
#include "ProviderAPI/StateHandler/DenyAllExternalControlHandler.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <thread>

// Change this to a unique EPR
const std::string PROVIDER_EPR("urn:uuid:sdcx-ORTableProvider-1234-12345");

// In case the provider shall be started without TLS, set this variable to false
constexpr bool ENABLE_TLS{true};

// Using definitions for increased readability 
using namespace Logging;
using namespace ProviderAPI;
using namespace ProviderAPI::StateHandler;
using namespace std::chrono_literals;


struct VirtualORTable
{
    double height; // 60-140cm
    double trend; // -45° till +45°
    double tilt; // -25° till +25°
    double backplate; // -40° till +80°
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

        p_transactionHandler->transitionFromEntryTo(UserInterfaces::Set::OnEntryInvocationState::Wait);
        p_transactionHandler->transitionFromWaitingTo(UserInterfaces::Set::OnWaitInvocationState::Start);
        p_transactionHandler->transitionFromStartedTo(UserInterfaces::Set::OnStartedInvocationState::Fin);
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
class ValueUpdater
{
private:
    ProviderAPI::SDCProvider* m_provider{nullptr};
    VirtualORTable* m_virtualORTable;

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
                // Randomly change data            
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
    
    // Discovery, DPWS and Get handlers are not specified in this simple example. The user could be informed about incoming Probe/Resolve/Get requests via these. See ReferenceProvider for 
    // implementation examples

    // register a callback for incoming activate messages. Activates are used to trigger actions - in this example the mode of the pulsoximeter is switched    
    auto setValueHandler = std::make_shared<DenyAllExternalControlHandler<SetOperationStatesContainer::SetValueStates>>();    
    auto setStringHandler = std::make_shared<DenyAllExternalControlHandler<SetOperationStatesContainer::SetStringStates>>();
    auto orTableActivateHandler = std::make_shared<ORTableActivateHandler>();
    auto orTableSetAlertStateHandler = std::make_shared<ORTableSetAlertStateHandler>();
    auto orTableSetContextStateHandler = std::make_shared<ORTableSetContextStateHandler>();
    auto orTableSetMetricStateHandler = std::make_shared<DenyAllExternalControlHandler<SetOperationStatesContainer::SetMetricStates>>();


    provider->registerSetValueExternalControlHandler(setValueHandler);
    provider->registerSetStringExternalControlHandler(setStringHandler);
    provider->registerActivateExternalControlHandler(orTableActivateHandler);
    provider->registerSetAlertStateExternalControlHandler(orTableSetAlertStateHandler);
    provider->registerSetMetricStateExternalControlHandler(orTableSetMetricStateHandler);
    provider->registerSetContextStateExternalControlHandler(orTableSetContextStateHandler);

    /*
    * 
    * RUNTIME
    * 
    */
    // after setting up everything, the provider can be started
    provider->startup();

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
