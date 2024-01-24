using UnityEngine;

//#define NETFX_CORE

#if NETFX_CORE
using Windows.Devices.Bluetooth.Advertisement;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Devices.Bluetooth;
using Windows.Devices.Bluetooth.GenericAttributeProfile;
using Windows.Storage.Streams;
#endif

// Class def
public class ReceiveBLE : MonoBehaviour
{

#if NETFX_CORE
    private BluetoothLEAdvertisementWatcher watcher;
    //public static ushort BEACON_ID = 1775;

    // UUIDs from our BLE server device
    private static Guid SERVICE_UUID = "56507bcc-dc2f-44b6-8d75-ab321779368c";
    private static Guid CHARACTERISTIC_UUID = "470d57b4-95d2-439b-a6cb-b1e68eb55352";

    // Global vars and flags
    private BluetoothAddressType deviceAddressType;
    private UInt64 deviceAddress;
    
    private BluetoothLEDevice bluetoothLeDevice;
    private GattCharacteristic selectedCharacteristic;

#endif
    public EventProcessor eventProcessor;
    private bool deviceFound = false;
    private bool deviceConnected = false;


    // Run on awake
    void Awake(){
#if NETFX_CORE
        deviceFound = false;

        // Use watcher to find our device
        watcher = new BluetoothLEAdvertisementWatcher();

        // Filter by Service UUID (our device should be the only using this UUID)
        var serviceID = new ServiceUuids{ServiceID = SERVICE_UUID};
        watcher.AdvertisementFilter.Advertisement.ServiceUuids.Add(ServiceID);

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

        
#endif
    }



    // Triggered when watcher received event calls back
#if NETFX_CORE
    private async void Watcher_Received(BluetoothLEAdvertisementWatcher sender, BluetoothLEAdvertisementReceivedEventArgs args)
    {

        // Add here 
        deviceAddress = args.BluetoothAddress;
        deviceAddressType = args.BluetoothAddressType;

        //ushort identifier = args.Advertisement.ManufacturerData[0].CompanyId;
        //byte[] data = args.Advertisement.ManufacturerData[0].Data.ToArray();

        // Updates to Unity UI don't seem to work nicely from this callback so just store a reference to the data for later processing.
        //eventProcessor.QueueData(data);

        // Update flag so we know we found our device. Make sure connect button is active when this flag triggers
        deviceFound = true;

        // Connect if we find our device
        //ConnectDevice(deviceAddress, deviceAddressType); // commented out to initiate from UI
    }
#endif

    public void Connect(){
    #if NETFX_CORE
        ConnectDevice(deviceAddress, deviceAddressType);
    #endif
    }

    // Connect to our device (to be triggered from UI
#if NETFX_CORE
    private async void ConnectDevice(UInt64 address, BluetoothAddressType addresstype){
    
        // Note: BluetoothLEDevice.FromIdAsync must be called from a UI thread because it may prompt for consent.
        bluetoothLeDevice = await BluetoothLEDevice.FromBluetoothAddressAsync(address, addresstype);

        // Immediately read from device for the service we want
        GattDeviceServicesResult result = await bluetoothLeDevice.GetGattServicesForUuidAsync(SERVICE_UUID);

        // Get the service from the list of services (should only be one)
        if (result.Status == GattCommunicationStatus.Success){
            var services = result.Services;

            // Get the list of characteristics from our service by UUID (should only be one)
            GattCharacteristicsResult result = await service.GetCharacteristicsForUuidAsync(CHARACTERISTIC_UUID);
            if (result.Status == GattCommunicationStatus.Success){
                var characteristics = result.Characteristics;
                selectedCharacteristic = characteristics.Item[0];

                // Read from characteristic just to get juices flowing
                GattReadResult result = await selectedCharacteristic.ReadValueAsync();
                if (result.Status == GattCommunicationStatus.Success){
                    // If that works, carry on.
                }
                else{
                    // Throw error
                }
            }
            else{
                var characteristics = null;
                // Throw error
            }
        }
        else{
            var services = null;
            //Throw error
        }


        // Subscribe to notify
        GattCommunicationStatus status = await selectedCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                        GattClientCharacteristicConfigurationDescriptorValue.Notify);
        if (status == GattCommunicationStatus.Success){
            // Server has been informed of clients interest.

            // Trigger flag to indicate device connected and subscribed!
            deviceConnected = true;

            // Link notify events to callback Characteristic_ValueChanged(); 
            characteristic.ValueChanged += Characteristic_ValueChanged; 
            
        }
        else{
            // Throw error
        }
    }
#endif



    // This runs when a Notify event triggers
#if NETFX_CORE
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
#if NETFX_CORE
    async void ReadData(){
        // Nothing here yet; we shouldn't need this if notify works correctly
    }
#endif

    // Disconnect from our device gracefully
#if NETFX_CORE
    public async void DisconnectDevice(){
        bluetoothLeDevice.Dispose();
    }
#endif

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