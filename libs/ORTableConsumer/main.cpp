/*
 * ORTableConsumer
 * 
 * @brief An example implementation to show a simple implmentation of the counterpart to the ORTableProvider.
 * 
 *
 *  Copyright @Surgitaix 2022
 */

#include "SDCCore/Core.h"
#include "ConsumerAPI/SDCConsumer.h"
#include "ConsumerAPI/Discovery.h"
#include "ConsumerAPI/Discovery/ConsumerDiscoveryMediator.h"
#include "ConsumerAPI/ConsumerCore/GetLayer/ConsumerGetMediator.h"
#include "ConsumerAPI/ConsumerCore/GetLayer/ConsumerGetHandler.h"
#include "ConsumerAPI/ConsumerCore/SetLayer/ConsumerSetHandler.h"
#include "ConsumerAPI/ConsumerCore/ReportingLayer/ConsumerReportingNotifier.h"

#include "Common/LinkUtilities.h"
#include "Threading/Threading.h"

#include "SDCCore/Prerequisites.h"

#include "ParticipantModel/PM/Mdib.h"

#include "Logging/LogBroker.h"
#include "Logging/Loggers/ConsoleLogger.h"
#include "Logging/Loggers/FileLogger.h"

#include "UserInterfaces/Get/ConsumerGet.h"
#include "UserInterfaces/Set/ConsumerSet.h"
#include "UserInterfaces/Set/InvocationStateConverter.h"

#include "ParticipantModel/PM/MdDescription.h"

#include "MessageModel/MSG/OperationInvokedReport.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>

using namespace Logging;

// Adapt accordingly
const std::string TARGET_EPR = "urn:uuid:sdcx-ORTableProvider-1234-12345";

std::unique_ptr<ConsumerAPI::SDCConsumer> consumer;


// builder for TLS config class
std::shared_ptr<Config::TLSConfig> createTLSConfig()
{
    auto tlsConfig = std::make_shared<Config::TLSConfig>();

    tlsConfig->setTrustedAuthorityLocation("./certificates/pat_ca.pem");
    tlsConfig->setCertificateLocation("./certificates/pat_cert.pem");
    tlsConfig->setPrivateKeyLocation("./certificates/pat_private.pem");

    return tlsConfig;
}

// callback function for reports with numeric metric state updates
void onNumericMetricStateUpdate(ParticipantModel::PM::NumericMetricState state)
{
    // in this example the received update is just output
    LogBroker::getInstance().log(LogMessage("ORTableConsumer",
                                            Severity::Notice,
                                            "Received NumericMetricState with descriptor handle: " + state.getDescriptorHandle().getValue()
                                                + " and value: " + state.getMetricValue()->getValue().getValue()));
}

// callback function for reports with alert updates in general (alert conditions, alert signals, ...)
void onAlert(MessageModel::MSG::EpisodicAlertReport p_data, UserInterfaces::Reporting::ReportingMetadata p_metadata)
{
    // skip empty reports and empty limit alert condition states 
    if(p_data.getReportPartList().empty())
    {
        return;
    }
    if(p_data.getReportPartList()[0].getLimitAlertConditionStateList().empty())
    {
        return;
    }
    for(const auto& alertState : p_data.getReportPartList()[0].getLimitAlertConditionStateList())
    {
        // in this example the received update is just displayed
        std::string presence = "false";
        if(alertState.getPresence().getValue())
        {
            presence = "true";
        }
        LogBroker::getInstance().log(
            LogMessage("ORTableConsumer",
                       Severity::Notice,
                       "Received episodic alert report with limit alert state: " + alertState.getDescriptorHandle().getValue()
        + ". Presence: " + presence));
    }

    // also reports of alert signals are expected in this example. Print them out or abort if not present
    if(p_data.getReportPartList()[0].getAlertSignalStateList().empty())
    {
        return;
    }
    for(const auto& alertState : p_data.getReportPartList()[0].getAlertSignalStateList())
    {
        // in this example the received update is just displayed
        std::string presence = "Off";
        if(alertState.getPresence() == ParticipantModel::PM::AlertSignalPresence::On)
        {
            presence = "On";
        }
        else if(alertState.getPresence() == ParticipantModel::PM::AlertSignalPresence::Latch)
        {
            presence = "Latching";
        }
        else if(alertState.getPresence() == ParticipantModel::PM::AlertSignalPresence::Ack)
        {
            presence = "Acknowledged";
        }
        LogBroker::getInstance().log(LogMessage("ORTableConsumer",
                       Severity::Notice,
                       "Received episodic alert report with alert signal with corresponding descriptor handle: "
                           + alertState.getDescriptorHandle().getValue() + " whose presence was changed to: " + presence));
    }

}


void onActivateResponse(UserInterfaces::Set::ConsumerSet::API::ActivateResponseReceived::Data_t p_data)
{
    LogBroker::getInstance().log(
        LogMessage("ORTableConsumer",
                   Severity::Notice,
                   "Received ActivateResponse with transaction id: " + std::to_string(p_data.getTransportMetadata()->getTransactionID())
                       + " from IP: " + p_data.getTransportMetadata()->getRemoteAddress() + " and current InvocationState: "
                       + UserInterfaces::Set::InvocationStateConverter::convertInvocationState(
                           p_data.getData()->getInvocationInfo().getInvocationState())));
}

// OperationInvokedReport received callback
void onOperationInvokedReport(UserInterfaces::Reporting::ConsumerReporting::API::OperationInvoked::Data_t p_data)
{

    LogBroker::getInstance().log(LogMessage("ORTableConsumer",
                                            Severity::Notice,
                                            "Received OperationInvokedReport for operation : " + p_data.getOperationHandleRef().getValue()
                                                + +"with current InvocationState: "
                                                + UserInterfaces::Set::InvocationStateConverter::convertInvocationState(
                                                    p_data.getInvocationInfo().getInvocationState())));
}

bool sendActivate(std::string operation)
{
    auto response = consumer->activate(operation);
    return response.getInvocationInfo().getInvocationState() != MessageModel::MSG::InvocationState::Fail;
}


int main(int argc, char* argv[])
{
    /*
    * 
    * SETUP
    * 
    */

    // network settings
    const std::string IP{""};
    const unsigned int PORT{10001};

    // Set local address
    std::shared_ptr<SDCCommon::DataTypes::NetworkAddress> localAddress{nullptr};

    auto networkInterface = SDCCore::getNetworkInterfaceByIPAddress(IP);
    if(networkInterface)
    {
        localAddress = std::make_shared<SDCCommon::DataTypes::NetworkAddress>(networkInterface->getIPv4Addresses()[0], PORT);
    }
    else
    {
        std::cout << "Could not bind to adapter with IP " << IP << ". Binding to default." << std::endl;
        networkInterface = std::make_unique<SDCCommon::DataTypes::NetworkInterface>(SDCCore::enumerateNetworkInterfaces()[0]);
        localAddress = std::make_shared<SDCCommon::DataTypes::NetworkAddress>(networkInterface->getIPv4Addresses()[0], PORT);
    }
    std::cout << "Binding to " << localAddress->toString() << std::endl;

    //// setup loggers
    std::string consoleLoggerTag = "console";
    std::string fileLoggerTag = "file";

    auto consoleLogger = std::make_shared<Logging::Loggers::ConsoleLogger>();
    auto fileLogger = std::make_shared<Logging::Loggers::FileLogger>("ORTableConsumer.log");

    Logging::LogBroker::getInstance().registerLogger(consoleLoggerTag, consoleLogger);
    Logging::LogBroker::getInstance().registerLogger(fileLoggerTag, fileLogger);
    Logging::LogBroker::getInstance().setLogLevel(Logging::Severity::Notice);

    // init the core of the framework
    LogBroker::getInstance().log(LogMessage("ORTableConsumer", Severity::Notice, "Creating Core..."));
    auto coreConfig = std::make_unique<Config::CoreConfig>();
    auto sdcCore = SDCCore::Core::createInstance(std::move(coreConfig));
    LogBroker::getInstance().log(LogMessage("ORTableConsumer", Severity::Notice, "Core created!"));

    // the discovery config contains all information for discovery. In most cases the default config is sufficiant.
    // The time for how long the discovery should take place can be set in this config
    auto discoveryConfig = std::make_shared<Config::DiscoveryConfig>(localAddress->getIPAddress());
    discoveryConfig->setMaxDiscoveryTime(std::chrono::milliseconds(3000));
    discoveryConfig->setDiscoverySendingEndpointPort(5012);
    auto discoveryService = ConsumerAPI::DiscoveryServiceFactory::createNew(sdcCore, discoveryConfig);

    // Create a new Handler for Discovery
    auto discoveryHandler = discoveryService->createDiscoveryHandler();


    // search for the device by EPR in the network
    try
    {
        auto targetEndpoint = discoveryHandler->resolve(TARGET_EPR).waitForResults();

        LogBroker::getInstance().log(LogMessage("ORTableConsumer", Severity::Notice, "Found EPR. Connecting to Provider"));
        // instantiate consumer class with the endpoint information from discovery
        auto consumerConfig = std::make_shared<Config::ConsumerConfig>(*localAddress, createTLSConfig());
        consumer = std::make_unique<ConsumerAPI::SDCConsumer>(sdcCore, *targetEndpoint, consumerConfig);
    }
    catch (const std::exception& e)
    {
        LogBroker::getInstance().log(LogMessage("ORTableConsumer", Severity::Notice, "Discovery failed: " + std::string(e.what())));
        return -1;
    }

    // Register callback for report notifications
    auto notifier = consumer->createReportingNotifier();
    notifier->registerNumericMetricStateUpdateCallback(onNumericMetricStateUpdate);
    notifier->registerOnEpisodicAlertReport(onAlert);
    notifier->registerOperationInvokedCallback(onOperationInvokedReport);

    // Register callback for SetValueResponse messages
    auto setHandler = consumer->createSetHandler();
    setHandler->registerActivateResponseCallback(onActivateResponse);

    bool exit = false;

    while (!exit)
    {
        std::cout << "OR Table demo consumer" << std::endl;
        std::cout << "a) increase table height" << std::endl;
        std::cout << "b) decrease table height" << std::endl;
        std::cout << "c) increase trend" << std::endl;
        std::cout << "d) decrease trend" << std::endl;
        std::cout << "e) increase tilt" << std::endl;
        std::cout << "f) decrease tilt" << std::endl;
        std::cout << "g) increase backplate" << std::endl;
        std::cout << "h) decrease backplate" << std::endl;
        std::cout << "i) Set predefined position to nullposition " << std::endl;
        std::cout << "j) Set predefined position to beach chair" << std::endl;
        std::cout << "k) Apply predefined position" << std::endl;

        std::cout << "y) Print status" << std::endl;
        std::cout << "z) Exit" << std::endl;
        std::cout << "Enter: ";


        char input;
        std::cin >> input;

        if (input == 'a')
        {
            sendActivate("MDC_OR_TABLE_ACTIVATE_INCREASE_TABLE_HEIGHT_SCO");
        }
        else if (input == 'b')
        {
            sendActivate("MDC_OR_TABLE_ACTIVATE_DECREASE_TABLE_HEIGHT_SCO");
        }
        else if (input == 'c')
        {
            sendActivate("MDC_OR_TABLE_ACTIVATE_INCREASE_TREND_SCO");
        }
        else if (input == 'd')
        {
            sendActivate("MDC_OR_TABLE_ACTIVATE_DECREASE_TREND_SCO");
        }
        else if (input == 'e')
        {
            sendActivate("MDC_OR_TABLE_ACTIVATE_INCREASE_TILT_SCO");

        }
        else if (input == 'f')
        {
            sendActivate("MDC_OR_TABLE_ACTIVATE_DECREASE_TILT_SCO");
        }
        else if (input == 'g')
        {
            sendActivate("MDC_OR_TABLE_ACTIVATE_INCREASE_BACK_SCO");
        }
        else if (input == 'h')
        {
            sendActivate("MDC_OR_TABLE_ACTIVATE_DECREASE_BACK_SCO");
        }
        else if (input == 'i')
        {
            consumer->setStringMetricValue("MDC_OR_TABLE_SETSTRING_PREDEFINED_POSITIONS_SCO", "NullLevel");
        }
        else if (input == 'j')
        {
            consumer->setStringMetricValue("MDC_OR_TABLE_SETSTRING_PREDEFINED_POSITIONS_SCO", "BeachChair");
        }
        else if (input == 'k')
        {
            sendActivate("MDC_OR_TABLE_ACTIVATE_APPLY_PREDEFINED_POSITION");
        }


        else if (input == 'y')
        {
            auto state = consumer->requestStates({"MDC_OR_TABLE_HEIGHT", "MDC_OR_TABLE_TREND", "MDC_OR_TABLE_TILT", "MDC_OR_TABLE_BACKPLATE" });
            // TODO PRINT
        }
        else if (input == 'z')
        {
            exit = true;
        }
    }



    LogBroker::getInstance().log(LogMessage("ORTableConsumer", Severity::Notice, "Shutting down"));
    LogBroker::getInstance().unregisterLogger(consoleLoggerTag);
    LogBroker::getInstance().unregisterLogger(fileLoggerTag);

    return 0;
   

}
