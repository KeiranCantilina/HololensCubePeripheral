using UnityEngine;
using System;

#if UNITY_WSA && !UNITY_EDITOR
using Windows.Devices.Bluetooth.Advertisement;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Devices.Bluetooth;
using Windows.Devices.Bluetooth.GenericAttributeProfile;
using Windows.Storage.Streams;
#endif

// Class def
public class ReceiveBLE : MonoBehaviour
{

#if UNITY_WSA && !UNITY_EDITOR
    private BluetoothLEAdvertisementWatcher watcher;
    //public static ushort BEACON_ID = 1775;

    // UUIDs from our BLE server device
    private static Guid SERVICE_UUID = Guid.Parse("56507bcc-dc2f-44b6-8d75-ab321779368c");
    private static Guid CHARACTERISTIC_UUID = Guid.Parse("470d57b4-95d2-439b-a6cb-b1e68eb55352");
    private static string DeviceName = "Cubii";

    // Global vars and flags
    private BluetoothAddressType deviceAddressType;
    private UInt64 deviceAddress;
    
    private BluetoothLEDevice bluetoothLeDevice;
    private GattCharacteristic selectedCharacteristic;
    //private GattDeviceService selectedService;

#endif
    public EventProcessor eventProcessor;
    private bool deviceFound = false;
    private bool deviceConnected = false;


    // Run on awake
    void Awake(){
        eventProcessor.DebugMessages("ReceiveBLE: Awake");
#if UNITY_WSA && !UNITY_EDITOR
        eventProcessor.DebugMessages("ReceiveBLE: Running UNITY_WSA");

        deviceFound = false;

        // Use watcher to find our device
        watcher = new BluetoothLEAdvertisementWatcher();

        // Filter by Service UUID (our device should be the only using this UUID)
        var ServiceID = SERVICE_UUID;
        watcher.AdvertisementFilter.Advertisement.ServiceUuids.Add(ServiceID);

        // Filter by device local name ("Cubii")
        //watcher.AdvertisementFilter.Advertisement.LocalName = DeviceName;

        // Set some settings
        watcher.AllowExtendedAdvertisements = true;
        watcher.ScanningMode = BluetoothLEScanningMode.Active;

        // Old stuff
        /*var manufacturerData = new BluetoothLEManufacturerData
        {
        CompanyId = BEACON_ID
        };
        watcher.AdvertisementFilter.Advertisement.ManufacturerData.Add(manufacturerData);*/

        // Tie watcher received data to the "Watcher_Received" function
        watcher.Received += Watcher_Received;
        
        // Start the watcher
        watcher.Start();

        // Throw log message up
        eventProcessor.DebugMessages("Device-Finder Status: " + watcher.Status.ToString());
        
#endif

    }



    // Triggered when watcher received event calls back
#if UNITY_WSA && !UNITY_EDITOR
    private void Watcher_Received(BluetoothLEAdvertisementWatcher sender, BluetoothLEAdvertisementReceivedEventArgs args)
    {
        // Debug pring
        eventProcessor.DebugMessages("ReceiveBLE: Watcher Received!");

        // Add here 
        deviceAddress = args.BluetoothAddress;
        deviceAddressType = args.BluetoothAddressType;

        //ushort identifier = args.Advertisement.ManufacturerData[0].CompanyId;
        //byte[] data = args.Advertisement.ManufacturerData[0].Data.ToArray();

        // Updates to Unity UI don't seem to work nicely from this callback so just store a reference to the data for later processing.
        //eventProcessor.QueueData(data);

        // Update flag so we know we found our device. Make sure connect button is active when this flag triggers
        deviceFound = true;

        // Print flags to log
        eventProcessor.PrintFlags(deviceFound, deviceConnected);

        // Connect if we find our device
        //ConnectDevice(deviceAddress, deviceAddressType); // commented out to initiate from UI
    }
#endif

    public void Connect(){
#if UNITY_WSA && !UNITY_EDITOR
        ConnectDevice(deviceAddress, deviceAddressType);
#endif
    }

    // Connect to our device (to be triggered from UI
#if UNITY_WSA && !UNITY_EDITOR
    private async void ConnectDevice(UInt64 address, BluetoothAddressType addresstype){
    
        // Note: BluetoothLEDevice.FromIdAsync must be called from a UI thread because it may prompt for consent.
        bluetoothLeDevice = await BluetoothLEDevice.FromBluetoothAddressAsync(address, addresstype);

        // Immediately read from device for the service we want
        GattDeviceServicesResult servicesresult = await bluetoothLeDevice.GetGattServicesForUuidAsync(SERVICE_UUID);

        // Get the service from the list of services (should only be one)
        if (servicesresult.Status == GattCommunicationStatus.Success){
            var services = servicesresult.Services;
            //selectedService = services[0];

            // Get the list of characteristics from our service by UUID (should only be one)
            GattCharacteristicsResult charresult = await services[0].GetCharacteristicsForUuidAsync(CHARACTERISTIC_UUID);
            if (charresult.Status == GattCommunicationStatus.Success){
                var characteristics = charresult.Characteristics;
                selectedCharacteristic = characteristics[0];

                // Read from characteristic just to get juices flowing
                GattReadResult readresult = await selectedCharacteristic.ReadValueAsync();
                if (readresult.Status == GattCommunicationStatus.Success){
                    // If that works, carry on.
                }
                else{
                    // Throw error
                }
            }
            else{
                //var characteristics = "";
                //var selectedCharacteristic = "";
                // Throw error
            }
        }
        else{
            //var services = "";
            var selectedCharacteristic = "";
            //Throw error
        }


        // Subscribe to notify
        GattCommunicationStatus Commstatus = await selectedCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                        GattClientCharacteristicConfigurationDescriptorValue.Notify);
        if (Commstatus == GattCommunicationStatus.Success){
            // Server has been informed of clients interest.

            // Trigger flag to indicate device connected and subscribed!
            deviceConnected = true;

            // Print to debug text
            eventProcessor.PrintFlags(deviceFound, deviceConnected);

            // Link notify events to callback Characteristic_ValueChanged(); 
            selectedCharacteristic.ValueChanged += Characteristic_ValueChanged; 
            
        }
        else{
            // Throw error
        }
    }
#endif



    // This runs when a Notify event triggers
#if UNITY_WSA && !UNITY_EDITOR
    void Characteristic_ValueChanged(GattCharacteristic sender, GattValueChangedEventArgs args){

        // An Indicate or Notify reported that the value has changed.
        var reader = DataReader.FromBuffer(args.CharacteristicValue);
        byte[] input = new byte[reader.UnconsumedBufferLength];
        reader.ReadBytes(input);

        // Shove the data into Event processor
        eventProcessor.QueueData(input);
    }
#endif

    // Probably won't ever be used
#if UNITY_WSA && !UNITY_EDITOR
    void ReadData(){
        // Nothing here yet; we shouldn't need this if notify works correctly
    }
#endif

    // Disconnect from our device gracefully
    public void Disconnect(){
#if UNITY_WSA && !UNITY_EDITOR
        //selectedService.Dispose();
        bluetoothLeDevice.Dispose();
        //Awake();
#endif
    }


    // Flag reading methods
    public bool ReadDeviceFoundFlag()
    {
        return deviceFound;
    }
    public bool ReadDeviceConnectedFlag()
    {
        return deviceConnected;
    }


}