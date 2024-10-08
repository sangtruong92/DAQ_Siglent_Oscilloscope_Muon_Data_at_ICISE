#include <iostream>
// #include <unistd.h>
#include <cstdlib>
#include <string>
#include <ctime>
#include <chrono>
#include <iomanip>

#include <stdio.h> // c-library

#include "OscController.h"
#include "AcqController.h"
#include <fstream>
#include <sstream>
#include "TGraph.h"
#include "TTimeStamp.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TApplication.h"
#include "TSystem.h"

using namespace std;

// rewrite code by structure


/*using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;*/
// valconvert= (val)*ydiv_c1/250./25.-voffset_c1 - ((val)*ydiv_c1/250./25. -voffset_c1)*0.1;
Float_t convert_voltage(Short_t val, double ydiv, double voffset){
    Float_t valconvert;
    if(val>127){val-=256;}
    valconvert = val*ydiv/25. - voffset;
    return valconvert;
}


class processing_class{
public:
    // vị trí đầu tiên, có thể bỏ nó trong code nếu cần, Drop out first value of minima
    vector<Double_t> data;
    vector<Double_t> data_Reduction;
    vector<Double_t> time_Reduction;
    vector<int> minima ;
    vector<Double_t> smoothed;
    // std::vector<int> left_minima;
    // std::vector<int> right_minima;

    void get_Data(const vector<Double_t> &inputdata)
    {
        data = inputdata;
    }

    // window size for scan data, having to use data in base class
    // if you want a funcion return vector , shoud use : std::vector<double> smoothData(int window_size)  
    // if you don't want a function return vector, that you just put value into the vector variable, shoud use void  
    void smooth_Data(int window_size) 
    {
        int half_window = window_size / 2;
        // std::vector<double> smoothed(data.size());
        for (size_t i = 0; i < data.size(); ++i) 
        {
            Float_t sum = 0.0;
            int count = 0;
            for (int j = -half_window; j <= half_window; ++j) 
            {
                if (i + j >= 0 && i + j < data.size()) 
                {
                    sum += data[i + j];
                    ++count;
                }
            }
            smoothed.push_back(sum / count);
        }
        // If you don't return, data will be saved by previous step, it's also okay, if don't return data should use void
        // return smoothed;
    }


    // Function to find minima in the data
    // std::vector<int> findMinima( double threshold) 
    void find_Minima( const vector<Double_t> &typeOf_Data, Float_t threshold ) 
    { 
        // bat dau tim dinh tu vi tri 100
        for (int i = 100; i < typeOf_Data.size() - 100; ++i)
        {
            if (typeOf_Data[i] <= threshold)
            {
                if ( (typeOf_Data[i] < typeOf_Data[i - 1]) && (typeOf_Data[i] < typeOf_Data[i + 1]) )
                {
                    minima.push_back(i);
                }
            }
        }


        // If you don't return, data will be saved by previous step, it's also okay 
        // return minima;
    }

    //  std::vector<double> data_reduction() , if return vector
    void reduction(int left_points, int right_points, double tdiv, double samplingrate)
    {
        Double_t temp;
        int scan_number;
        // bỏ đỉnh đầu tiên
        for (int i = 1; i < minima.size(); i++)
        {
            for (int j = 0; j < (left_points + right_points) ; j++)
            {
                // (minima[i] > left_points ) &&               // vị trí đỉnh lớn số chân trái 
                // (minima[i] < data.size() - 50 ) &&          // đỉnh phải nhỏ hơn kích thước dữ liệu - 50

                if ((minima[i] > minima[i-1] + right_points )) // đỉnh sau lớn hơn vị trí chân phải đỉnh trước + decay time
                {
                    scan_number = (minima[i] - left_points) + j;
                    temp = data[scan_number];
                    data_Reduction.push_back(temp);
                    time_Reduction.push_back(-tdiv*14./2.+ scan_number/samplingrate);//in [s] for 5E+8 Sap/s
                    // left_minima.push_back(i-left_points);
                    // right_minima.push_back(i+right_points);
                }
            }
        }
        // If you don't return, data will be saved by previous step, it's also okay, if you function return vector let's to use it
        // return data_Reduction
    }

    void clearVector() {
        // This clears the vector and releases memory
        data.clear();
        data_Reduction.clear();
        time_Reduction.clear();
        minima.clear();
        smoothed.clear();
    }

};



int main(int argc, char const *argv[]) {
    cout << "*********OscAcq*********\n" << endl;
    
    // if argv != 4 then print ./OscAcq [Oscilloscope IP] [Samples] [Output filename], end programe.
    // if argv = 4, then process code. 

    if (argc != 4) {
        cout << "Usage: ./OscAcq [Oscilloscope IP] [Samples] [Output filename]" << endl;
        
        return 0;
    }
    
    long samples = atol(argv[2]);
    
    string fname= argv[3];
    
    //buffer size must be appropriate
    //OscController osc(argv[1], samples*4+80000); //for 4 channels
    OscController osc(argv[1], samples*2+80000);
    osc.Initialize();
    
    //check oscilloscope ID
    cout << "Oscilloscope ID:" << endl;
    osc.askOscilloscopeID();
    osc.ReceiveMessage();
    cout << osc.convertBufferToString() << endl;
    
    //Setting for acquition
    //this adding channels has no meaning yet
    AcqController acq(&osc);
    acq.addChannel("1");
    acq.setSamples(samples);
    acq.Initialize(argv[3]);
    
    //start then stop
    //osc.start();
    //osc.wait();
    //sleep(1);//sleeping 1s probably too long? 0.7s still has some broken waveforms
    //osc.stop();
    
    //check fundamental information
    osc.SendAndReceive("C1:VDIV?");//vertial division
    //cout <<osc.convertBufferToString()<<endl;
    Float_t ydiv_c1;
    char IgnoredStr[20];
    int rc = sscanf(osc.convertBufferToString().c_str(), "%s %fV", IgnoredStr, &ydiv_c1);
    cout <<"Vertical division channel 1 in [V] "<<ydiv_c1<<endl;
 
    
    Float_t voffset_c1;
    osc.SendAndReceive("C1:OFST?");//vertial division
    //cout <<osc.convertBufferToString()<<endl;
    int rc1 = sscanf(osc.convertBufferToString().c_str(), "%s %fV", IgnoredStr, &voffset_c1);
    cout<<"V offset channel 1 in [V] "<<voffset_c1<<endl;
   
    
    //for channel 2
    osc.SendAndReceive("C2:VDIV?");//vertial division
    Float_t ydiv_c2;
    int rc_c2 = sscanf(osc.convertBufferToString().c_str(), "%s %fV", IgnoredStr, &ydiv_c2);
    cout<<"Vertical division channel 2 in [V] "<<ydiv_c2<<endl;
    
    Float_t voffset_c2;
    osc.SendAndReceive("C2:OFST?");//vertial division
    int rc1_c2 = sscanf(osc.convertBufferToString().c_str(), "%s %fV", IgnoredStr, &voffset_c2);
    cout<<"V offset channel 2 in [V] "<<voffset_c2<<endl;
    

    //for channel 3
    osc.SendAndReceive("C3:VDIV?");//vertial division
    Float_t ydiv_c3;
    int rc_c3 = sscanf(osc.convertBufferToString().c_str(), "%s %fV", IgnoredStr, &ydiv_c3);
    cout<<"Vertical division channel 3 in [V] "<<ydiv_c3<<endl;
    
    Float_t voffset_c3;
    osc.SendAndReceive("C3:OFST?");//vertial division
    int rc1_c3 = sscanf(osc.convertBufferToString().c_str(), "%s %fV", IgnoredStr, &voffset_c3);
    cout<<"V offset channel 3 in [V] "<<voffset_c3<<endl;
    
    
    //for channel 4
    osc.SendAndReceive("C4:VDIV?");//vertial division
    Float_t ydiv_c4;
    int rc_c4 = sscanf(osc.convertBufferToString().c_str(), "%s %fV", IgnoredStr, &ydiv_c4);
    cout<<"Vertical division channel 4 in [V] "<< ydiv_c4 <<endl;
    
    Float_t voffset_c4;
    osc.SendAndReceive("C4:OFST?");//vertial division
    int rc1_c4 = sscanf(osc.convertBufferToString().c_str(), "%s %fV", IgnoredStr, &voffset_c4);
    cout<<"V offset channel 4 in [V] "<< voffset_c4 <<endl;
    

    
    
    //Timing offset
    osc.SendAndReceive("TRDL?");//vertial division
    //cout <<osc.convertBufferToString()<<endl;
    double t0trig;
    int rct0 = sscanf(osc.convertBufferToString().c_str(), "%s %lfS", IgnoredStr, &t0trig);
    cout<<"Trigger time in [s] "<<t0trig<<endl;
    
    //Timing division
    osc.SendAndReceive("TDIV?");//vertial division
    //cout <<osc.convertBufferToString()<<endl;
    double tdiv;
    int rct1 = sscanf(osc.convertBufferToString().c_str(), "%s %lfS", IgnoredStr, &tdiv);
    cout<<"Time per division [s] "<<tdiv<<endl;

    /***************remote trigger setup*****************/
    /*
    //check trigger setup
    cout<<"Check trigger type: "<<endl;
    osc.SendAndReceive("TRSE?");
    cout <<osc.convertBufferToString()<<endl;
    if(osc.convertBufferToString().find("EDGE") == std::string::npos) { //if not edge type
    osc.SendMessage("TRSE EDGE,SR,C1,HT,OFF"); 
    }
    cout<<"Set trigger type of C1 to edge: "<<endl;
    osc.SendAndReceive("TRSE?");
    cout <<osc.convertBufferToString()<<endl;

    //check trigger setup
    cout<<"Check and set trigger slope for C1 to rising: "<<endl;
    osc.SendAndReceive("C1:TRSL?");
    cout <<osc.convertBufferToString()<<endl;
    if(osc.convertBufferToString().find("POS") == std::string::npos) { //if not rising slope
    osc.SendMessage("C1: TRSL POS"); 
    }

    //Set trigger level
    cout<<"Set trigger level for C1: "<<endl;
    osc.SendMessage("C1:TRLV 3V");//if in mV then "C1:TRLV 3000MV"
    osc.SendAndReceive("C1:TRLV?");
    cout <<osc.convertBufferToString()<<endl;
    */
    /***************remote trigger setup*****************/
    // //check trigger pattern 
    //cout<<"Check trigger pattern: "<<endl;
    //osc.SendAndReceive("TRPA?");
    //cout <<osc.convertBufferToString()<<endl;
    // //when in trigger pattern type, C2 & C3 are on, set C2 & C3 to low and condition AND
    // osc.SendMessage("TRPA C2,L,C3,L,STATE,AND"); 

    //sampling rate
    osc.SendAndReceive("SARA?");//vertial division
    //cout<<"Sampling rate "<<endl;
    //cout <<osc.convertBufferToString()<<endl;
    double samplingrate;
    int rct2 = sscanf(osc.convertBufferToString().c_str(), "%s %lfSa/s", IgnoredStr, &samplingrate);
    cout<<"Sampling rate [s] "<<samplingrate<<endl;
    //SARA 5.00E+08Sa/s for SDS1104X-E/SDS1204X-E
    
    //record the waveform
    //check waveform setup
    cout<<"Waveform Setup "<<endl;
    osc.SendAndReceive("WaveForm_SetUp?");//make sure all points are recorded
    cout <<osc.convertBufferToString()<<endl;
    
    //("WFSU SP,0,NP,0,F,0")
    
    
    //    Number of sampling point
    cout<<"Sampling point C1"<<endl;
    osc.SendAndReceive("SANU? C1");//vertial division
    cout <<osc.convertBufferToString()<<endl;
    //SANU 7.00E+04pts for SDS1104X-E; 7.00E+03pts for SDS1204X-E;
    double tmpdatasize;// = 7000;//osc.getBufferSize();
    int rc2 = sscanf(osc.convertBufferToString().c_str(), "%s %lfpts", IgnoredStr, &tmpdatasize);
    long datasize = (long)tmpdatasize;
    cout <<"C1: No. of data point "<<datasize<<endl;
    
    //test for Channel 2
    osc.SendMessage("C2:TRA ON");//Make sure Channel 2 trace is on
    osc.SendAndReceive("C2:TRA?");//vertial division
    cout <<osc.convertBufferToString()<<endl;
    
    osc.SendMessage("C3:TRA ON");//Make sure Channel 2 trace is on
    osc.SendAndReceive("C3:TRA?");//vertial division
    cout <<osc.convertBufferToString()<<endl;
    
    osc.SendMessage("C4:TRA ON");//Make sure Channel 2 trace is on
    osc.SendAndReceive("C4:TRA?");//vertial division
    cout <<osc.convertBufferToString()<<endl;
   
    //seems SANU command for C2 got problem
    //cout<<"Sampling point  C2"<<endl;
    /*osc.SendAndReceive("SANU? C2");//vertial division
     cout <<osc.convertBufferToString()<<endl;
     //SANU 7.00E+04pts for SDS1104X-E
     double tmpdatasize_c2;// = 70000;//osc.getBufferSize();
     int rc2_c2 = sscanf(osc.convertBufferToString().c_str(), "%s %lfpts", IgnoredStr, &tmpdatasize_c2);
     long datasize_c2 = (long)tmpdatasize_c2;
     cout <<"C2: No. of data point "<<datasize_c2<<endl;*/
    
        
    TFile *outFile = new TFile(fname.data(), "RECREATE");
    
    outFile->cd();
    TTree *dataTree = new TTree("dataTree", "dataTree");
    
    // Long64_t currentEventNumber;
    Double_t squence_Start_Sec_And_NanoSec;
    Double_t squence_End_Sec_And_NanoSec;
    Double_t first_Event_Trigger_Sec_NanoSec;
    Double_t realtime_Event_Conversion_Sec_And_NanoSec;
    Double_t time_Frame_Conversion_Sec_And_NanoSec;
    Double_t transfer_Start_Time_Convertion_Sec_And_NanoSec;//in nanosecond second
    Double_t transfer_end_Convertion_Sec_And_NanoSec;
    Double_t convertVolt_End_Convertion_Sec_And_NanoSec;
    Double_t time_Sequence_Recording;
    Double_t muon_Rate_Averate;

    //Long64_t epochTimeNanoSec_trg_stop1;
    //Long64_t timeEventSec;//in second
    // Long64_t frameMiniNanoSec;

    // dataTree->Branch("EventNumber", &currentEventNumber, "EventNumber/L");
    // careful with name of variable, space, Caple 
    // Note: All elements of Banch is same length event you just caculate one time for one any variable. 
    dataTree->Branch("Squence_Start_Sec_And_NanoSec", &squence_Start_Sec_And_NanoSec, "Squence_Start_Sec_And_NanoSec/D"); 
    dataTree->Branch("Squence_End_Sec_And_NanoSec", &squence_End_Sec_And_NanoSec, "Squence_End_Sec_And_NanoSec/D");
    dataTree->Branch("First_Event_Trigger_Sec_NanoSec", &first_Event_Trigger_Sec_NanoSec, "First_Event_Trigger_Sec_NanoSec/D");
    dataTree->Branch("Realtime_Event_Conversion_Sec_And_NanoSec", &realtime_Event_Conversion_Sec_And_NanoSec, "Realtime_Event_Conversion_Sec_And_NanoSec/D"); 
    dataTree->Branch("Time_Frame_Conversion_Sec_And_NanoSec", &time_Frame_Conversion_Sec_And_NanoSec, "Time_Frame_Conversion_Sec_And_NanoSec/D"); //in nanosecond
    dataTree->Branch("Transfer_Start_Time_Convertion_Sec_And_NanoSec", &transfer_Start_Time_Convertion_Sec_And_NanoSec, "Transfer_Start_Time_Convertion_Sec_And_NanoSec/D"); //in nanosecond
    dataTree->Branch("Transfer_end_Convertion_Sec_And_NanoSec", &transfer_end_Convertion_Sec_And_NanoSec,"Transfer_end_Convertion_Sec_And_NanoSec/D");
    dataTree->Branch("ConvertVolt_End_Convertion_Sec_And_NanoSec", &convertVolt_End_Convertion_Sec_And_NanoSec,"ConvertVolt_End_Convertion_Sec_And_NanoSec/D");
    dataTree->Branch("Time_Sequence_Recording", &time_Sequence_Recording,"Time_Sequence_Recording/D");
    dataTree->Branch("Muon_Rate_Averate", &muon_Rate_Averate,"Muon_Rate_Averate/D");


     //dataTree->Branch("EpochTimeNanoSec_trg_stop1", &epochTimeNanoSec_trg_stop1, "EpochTimeNanoSec_trg_stop1/L"); //in nanosecond
    //dataTree->Branch("TimeEventSec", &timeEventSec, "TimeEventSec/L"); //in second
    // dataTree->Branch("frameMiniNanoSec", &frameNanoSec, "frameNanoSec/L"); //in nanosecond
    // dataTree->Branch("TimeEventNanoSec", &timeEventNanoSec, "TimeEventNanoSec/L");//in second
    
    // dataTree->AutoSave();
    
    //sampling rate
    //osc.SendAndReceive("C1:WF? DAT2");//vertial division
    //turn off the header
    cout<<" header to be SHORT "<<endl;
    osc.SendMessage("CHDR SHORT");//or ALL
    osc.SendAndReceive("CHDR?");
    cout << osc.convertBufferToString() << endl;
    osc.setSingleMode();//set single mode
    //taking data
    int j = 0;
    double tvalue[datasize];
    // Float_t valconvert1, valconvert2, valconvert3, valconvert4;
    vector<Double_t> val1, val2, val3, val4;
    /*std::time_t atime;
    std::tm* anow;
    atime = std::time(0);   // get time now
        anow = std::localtime(&atime);
        std::cout << (anow->tm_year + 1900) << '-'
             << (anow->tm_mon + 1) << '-'
             <<  anow->tm_mday
             << "\n";*/

    // TTimeStamp *timeNow = new TTimeStamp();
    // Double_t valtimenow = timeNow->GetSec()+timeNow->GetNanoSec()*1e-9;
   // cout<<"Time now "<< std::setprecision(20)<<valtimenow<<endl;
    
    //test the DAQ first
    //osc.start();
    //osc.wait();
    //osc.stop();
    //cout<<"finish test the DAQ"<<endl;
    // int ioscstatus;
    //check the trigger status
    /*iosc.SendAndReceive("INR?");
        cout << osc.convertBufferToString() << endl;
        int rcx0 = sscanf(osc.convertBufferToString().c_str(), "INR %lf", &tmpdatasize);
        ioscstatus = (int)tmpdatasize;*/

    TApplication* theApp = new TApplication("App", 0, 0);
    // TCanvas* c1 = new TCanvas();
    // TCanvas* c2 = new TCanvas();
    // // TCanvas* c3 = new TCanvas();
    // // TCanvas* c4 = new TCanvas();

    // Time_Dif_Frame Depend on frequence : 100k frequence set 0.01s - test with generator
    TH1D *time_Dif_Frame = new TH1D("Time_Dif_Frame", "", 1000, 0.0, 10);     // muon envent (1000,0,10) . test (10000, 0, 0.01)         
    TH1D *time_Transfer_Data = new TH1D("Time_Transfer_Data", "", 1000, 0.0, 1);   // muon envent (1000,0,1) . test (1000,0,1)
    // TH1D *trigger_time = new TH1F("trigger_time", "", 200, 0.0, 2.0); //triggering time from start to stop osc
    TH1D *time_Convert_Data = new TH1D("Time_Convert_Data", "", 1000, 0.0, 0.01);    // muon envent (1000,0,0.1) . test (1000, 0, 0.1)


     // change oscilloscope to single mode
    osc.setSingleMode();
    //usleep(100000); //microsecond
    // The ARM_ACQUISITION command starts a new signal acquisition, chuyển từ single mode(Stop) sang acquision để lấy dữ liệu mới
    // change trigger from stopped to sigle, after one loop  
    osc.SendMessage("ARM");

    //usleep(100000); //microsecond   
    // begin to take data
    // for (Int_t isample=0; isample<samples; ++isample) 
    // {
    //  currentEventNumber = isample +1;
    //  cout <<"Event "<<currentEventNumber<<"/"<<samples<< endl;
    cout<< " \n Stoping t0:" << endl;

    
    cout<<" Osc status "<<endl;
    osc.SendAndReceive("SAST?");//make sure all points are recorded
    cout << osc.convertBufferToString()<<endl;
    // usleep(100000); //microsecond



        // Begin sequence mode: 
        // auto start_squence = std::chrono::high_resolution_clock::now();
        // auto nanosecond_squence_start = std::chrono::time_point_cast<std::chrono::nanoseconds>(start_squence).time_since_epoch().count();
        // cout << "nanosecond_squence_start : " << nanosecond_squence_start << endl;


        // Time now : https://root.cern.ch/doc/v628/classTTimeStamp.html#a517698f238587bc760c87aa444053cfc
        TTimeStamp *time_Temp_Set_Again = new TTimeStamp(); // every time you set it will be changed
        time_t squence_Start_Sec;
        Int_t squence_Start_NanoSec;
        time_t time_Zone_Offset = TTimeStamp::GetZoneOffset(); // Get time Offset from vietnam -25200
        cout << "time_Zone_Offset: " << time_Zone_Offset << endl; 
        time_Temp_Set_Again->Set(); // If you do set(), it understand now time from system. if you don't set, it is set() finally
        
        // time_Temp_Set_Again->Set(1970,1,1,7,0,0,0,kTRUE,0); // year-1970, month-1, day-1, hour-7, min-0, sec-0,nsec-0, isUTC-kTRUE, secOffset-0 
        squence_Start_Sec = time_Temp_Set_Again->GetSec() - time_Zone_Offset; // Second from 1970 to now at 0h local time + 
        squence_Start_NanoSec = time_Temp_Set_Again->GetNanoSec(); // nano second from now
        // cout << squence_Start_Sec <<endl;
        // cout << squence_Start_NanoSec <<endl;

        TTimeStamp squence_Start_Convertion(squence_Start_Sec, squence_Start_NanoSec ); // TTimeStamp (time_t t, Int_t nsec)
        squence_Start_Sec_And_NanoSec = squence_Start_Convertion.AsDouble();
        cout << " squence_Start_Convertion: " << squence_Start_Convertion << endl;

        vector< pair< time_t, Int_t> > frame_time;
        time_t first_Event_Sec;
        Int_t first_Event_NanoSec;
        time_t first_FrameDT_Event_Sec ;
        Int_t first_FrameDT_Event_NanoSec;

        // osc.stop();
        // usleep(100000); //microsecond
        //osc.SendAndReceive("SAST?");//make sure all points are recorded
        int i_trigger = 0 ; 
        while(osc.convertBufferToString().find("Stop") == std::string::npos)
        {   //while the output is not "Stop"
            usleep(10); //microsecond
            // ask oscilloscope
            osc.SendAndReceive("INR?"); // feed back lastMsgLen
            int temp_status;
            int scan_status = sscanf(osc.convertBufferToString().c_str(), "INR %d", &temp_status);
	        int osc_status = (int)temp_status;

           // cout << (osc_status ==0)<<endl;
           //  cout << osc_status << endl;

            if (osc_status== 8193){ 
                i_trigger++;
                //////////////////////// Real_time_event//////////////////////////////
                time_Temp_Set_Again->Set(); // If you do set(), it understand now time from system. if you don't set, it is finally set()
                time_t realtime_Event_Sec;      // realtime = computertime
                Int_t realtime_Event_NanoSec;
                realtime_Event_Sec = time_Temp_Set_Again->GetSec() - time_Zone_Offset; // Second from 1970 to now at 0h local time + 
                realtime_Event_NanoSec = time_Temp_Set_Again->GetNanoSec(); // nano second from now
                TTimeStamp realtime_Event_Convertion(realtime_Event_Sec, realtime_Event_NanoSec );
                realtime_Event_Conversion_Sec_And_NanoSec = realtime_Event_Convertion.AsDouble();
                // firt frame
                // frame_time.push_back( make_pair( realtime_Event_Sec, realtime_Event_NanoSec ) );
                //////////// Because oscilloscope is faster than speed of comunication so take data and frame time will be percise.
                cout << "osci triggering: "<< i_trigger << " \t time of events: "<< realtime_Event_Convertion <<endl;
                
                ///////////////////////////first_event///////////////////////////////////////////
                if(i_trigger==1){
                    first_Event_Sec = realtime_Event_Sec;
                    first_Event_NanoSec = realtime_Event_NanoSec;
                    TTimeStamp first_Event_Trigger_Convertion(first_Event_Sec, first_Event_NanoSec );
                    first_Event_Trigger_Sec_NanoSec = first_Event_Trigger_Convertion.AsDouble();
                    cout << "first_Event_Trigger: " << first_Event_Trigger_Convertion << endl;
                    first_FrameDT_Event_Sec = (realtime_Event_Sec - squence_Start_Sec);
                    first_FrameDT_Event_NanoSec = (realtime_Event_NanoSec - squence_Start_NanoSec);
                }
                // else if(i_trigger>samples){
                //     break; 
                // }
            }

            osc.SendAndReceive("SAST?");//make sure all points are recorded, return the acquisition status of scope
            if(osc.convertBufferToString().find("Stop") != std::string::npos) { 
            //if the output is "Stop"
            //goto IfOscStop2;
            cout << "stop at if" << endl;
            break;
            } //end if
            // cout<<" Osc status after while "<<endl;

        } //end while
    
        // end sequence mode: 
        // The std::chrono::high_resolution_clock::now() function in C++ returns a time_point
        // auto end_squence = std::chrono::high_resolution_clock::now();
        // auto nanosecond_squence_end = std::chrono::time_point_cast<std::chrono::nanoseconds>(end_squence).time_since_epoch().count();
        // cout << "nanosecond_squence_end : " << nanosecond_squence_end << endl;

       

        cout<< "Stoping t1:" << endl;
        // TTimeStamp *time_Temp_Set_Again = new TTimeStamp(); // every time you set it will be changed
        time_t squence_End_Sec;
        Int_t squence_End_NanoSec;
        time_Temp_Set_Again->Set();
        squence_End_Sec = time_Temp_Set_Again->GetSec()- time_Zone_Offset; // Second from 1970 to now
        squence_End_NanoSec = time_Temp_Set_Again->GetNanoSec(); // nano second from now
        TTimeStamp squence_End_Convertion(squence_End_Sec, squence_End_NanoSec ); // TTimeStamp (time_t t, Int_t nsec)
        squence_End_Sec_And_NanoSec = squence_End_Convertion.AsDouble();
        // cout << " squence_End_Convertion: " <<squence_End_Convertion << endl;

        ////////////////////////////// Muon Rate /////////////////////////////////////////
        time_Sequence_Recording = (squence_End_Sec_And_NanoSec-first_Event_Trigger_Sec_NanoSec);
        muon_Rate_Averate = samples/(squence_End_Sec_And_NanoSec-first_Event_Trigger_Sec_NanoSec);  

        // turn on History mode and History List
        osc.SendMessage("HSMD ON");
        osc.SendMessage("HSLST ON"); // probably don't need this code to take data

        // // ask oscilloscope Current_Frame
        // osc.SendAndReceive("FPAR?");//make sure all points are recorded, return the acquisition status of scope        
        // // cout<< osc.convertBufferToString() << endl;
        // char *parameter = osc.getBuffer();
        // char temp_para [100];

        // for (int i =0 ; i< osc.getBufferSize() ; i++) {
        //     temp_para[i] = parameter[i];
        //     cout << temp_para[i] <<endl;
        // }
        // // first element of time_Frame_Conversion_Sec_And_NanoSec
        // // time_Frame_Conversion_Sec_And_NanoSec = first_Event_Conversion_Sec_And_NanoSec;
        

        
        
        // first frame time - different first trigger and sequence start, if first event speed comunication error it's time smaller than  
        cout << "samples: " << samples << endl; 
        frame_time.push_back( make_pair( first_FrameDT_Event_Sec , first_FrameDT_Event_NanoSec  ) );
        time_Dif_Frame->Fill( first_FrameDT_Event_Sec + first_FrameDT_Event_NanoSec*1e-9 );
        TTimeStamp time_Frame_Conversion(first_Event_Sec, first_Event_NanoSec );
        time_Frame_Conversion_Sec_And_NanoSec = time_Frame_Conversion.AsDouble();
        cout << "First_time_Frame_Conversion:"<< time_Frame_Conversion << endl;



    // Change Frame to read data, Time_DT= (frame2 - frame1) 
    for (int i_frame =0 ; i_frame < samples ; i_frame++ )
    {
        // read temporary frame: first fram begin 1.
        string tempo_frame = "Fram " + to_string(i_frame+1);
        osc.SendMessage(tempo_frame);
        // cout  << "Frame Of History:" << osc.convertBufferToString()<<endl;
        cout<< "\n Stoping t2:"  << endl;
        cout  << "Number of Frame: " << tempo_frame <<endl;
    
        //////////// Frame-time ///////////////////////////
        
        osc.SendAndReceive("FTIM?");
        // FTIM hour: minute: second. micro-second
        // FTIM 00: 18: 39. 189570 : %s %d %d %f %d , dinh dang khi ask oscilloscope : 
        // int sscanf(const char *str, const char *format, ...),  sscanf( dtm, "%s %s %d  %d", weekday, month, &day, &year );
        char nameFrame[20], frameHour[20], frameMin[20], frameSec[20], frameMicroSec[20];
        int frtime_value = sscanf(osc.convertBufferToString().c_str(), "%s %s %s %s %s ", nameFrame, frameHour, frameMin, frameSec, frameMicroSec);
        printf("%s %s %s %s %s \n", nameFrame, frameHour, frameMin, frameSec, frameMicroSec);
        
        int temp0_Second = (atoi(frameSec)  + atoi(frameMin)*60 +  atoi(frameHour)*60*60 ); // atol: char to long, atoi: char to int 
        time_t temp_Second = temp0_Second;
        Int_t temp_NanoSecond = atoi(frameMicroSec)*1000;
        // cout <<"temp_Second : " << temp_Second << endl;
        // cout <<"temp_NanoSecond : " << temp_NanoSecond<< endl;


        ///// caculate frame_time in second
        frame_time.push_back( make_pair( temp_Second, temp_NanoSecond ) );

        // cout << "temptime: " << temptime << endl;

        //  cout << "frameMicroSec: "<< atol(frameMicroSec)*1e-6 << endl;
        //  cout << "Frame Sec: "   << atol(frameSec) << endl;
        //  cout << "frameMin: "    << atol(frameMin)*60 << endl;
        //  cout << "frameHour: "    << atol(frameHour)*60*60 << endl;

        // cout <<  (atol(frameMicroSec)*1e-6 -'0' + atol(frameSec)  -'0' + atol(frameMin)*60  -'0' +  atol(frameHour)*60*60  -'0' )<< endl;
        // cout <<"frame time :" <<frame_time[i_frame] << endl;
        
        ///////////////////////// Time different between Frame in Oscilloscope ///////////////////
        if(i_frame >0){

        // frame_time[0]: First trigger, frame_time[1]: first event of oscilloscope, frame_time[2]: second event of scilloscope 
        time_t frameDT_Second =  frame_time[i_frame+1].first - frame_time[1].first;
        // Int_t frameDT_NanoSecond = (frame_time[i_frame-1].second - frame_time[i_frame-2].second); // Detatime in nanosecond convert from microsecond
        Int_t frameDT_NanoSecond = (frame_time[i_frame+1].second - frame_time[1].second);
        cout <<"frameDT_Second :" <<  frameDT_Second  << " frameDT_NanoSecond :" << frameDT_NanoSecond  << endl; 

        // i
        time_t frameDT_Second_fill =  frame_time[i_frame+1].first - frame_time[i_frame].first;
        Int_t frameDT_NanoSecond_fill = (frame_time[i_frame+1].second - frame_time[i_frame].second);
            cout <<" frame_Time_Difference_Second: " << frameDT_Second_fill  <<endl;
        // cout <<" frame_Time_Difference_NanoSecond: " << frameDT_NanoSecond_fill*1e-9  <<endl;
        
        // TGraph Fill Time Difference
        time_Dif_Frame->Fill( frameDT_Second_fill + frameDT_NanoSecond_fill*1e-9 );
        // cout <<" frame_Time_Difference_Second: " << frameDT_Second   <<endl;
        // cout <<" frame_Time_Difference_NanoSecond: " << frameDT_NanoSecond   <<endl;
        TTimeStamp time_Frame_Conversion(first_Event_Sec + frameDT_Second, first_Event_NanoSec+ frameDT_NanoSecond );
        time_Frame_Conversion_Sec_And_NanoSec = time_Frame_Conversion.AsDouble();
        // cout << time_Frame_Conversion.AsDouble() << endl;
        // cout << "time_Frame_Conversion.AsDouble:"<< setprecision(20) << time_Frame_Conversion_Sec_And_NanoSec<< endl;
        cout << "time_Frame_Conversion:"<< time_Frame_Conversion << endl;
        }
        


        // deltaTimeOfFrame =  
        // osc.SendAndReceive("SAST?");//make sure all points are recorded
        // cout<<"Osc status "<<endl;
        // osc.SendAndReceive("SAST?");//make sure all points are recorded
        // cout <<osc.convertBufferToString()<<endl;

        TGraph *pwfgraph_c1 = new TGraph();
        TGraph *pwfgraph_c2 = new TGraph();
        TGraph *pwfgraph_c3 = new TGraph();
        TGraph *pwfgraph_c4 = new TGraph();

        cout<< "Stoping t3:" << endl;
        time_t transfer_Start_Sec;
        Int_t transfer_Start_NanoSec;
        time_Temp_Set_Again->Set();
        transfer_Start_Sec = time_Temp_Set_Again->GetSec()- time_Zone_Offset; // Second from 1970 to now
        transfer_Start_NanoSec = time_Temp_Set_Again->GetNanoSec(); // nano second from now
        TTimeStamp transfer_Start_Time_Convertion(transfer_Start_Sec, transfer_Start_NanoSec ); // TTimeStamp (time_t t, Int_t nsec)
        transfer_Start_Time_Convertion_Sec_And_NanoSec = transfer_Start_Time_Convertion.AsDouble();
        // cout <<" transfer_Start_Time_Convertion: " << transfer_Start_Time_Convertion << endl;

            
        // double convert_voltage(int val,double ydiv, double voffset);

        //is this correct?
        // for c1. scan buffer 
        osc.SendMessage("C1:WF? DAT2");//or ALL
        osc.ReceiveMessage();

        // val = *((Short_t*) c);
        // trong qua trinh ReceiveMessage Oscilloscpe chi send data
        char *c1 = osc.getBuffer();
        for (int i =0 ; i< datasize ; i++) {
        val1.push_back( convert_voltage((Short_t)c1[i], ydiv_c1, voffset_c1) );
        }
        
        // cout << c1[1] << endl;
        // cout << convert_voltage(c1[1], ydiv_c1, voffset_c1) << endl;
        
        //for C2
        osc.SendMessage("C2:WF? DAT2");//or ALL
        osc.ReceiveMessage();
        char *c2 = osc.getBuffer();
        for (int i =0 ; i< datasize ; i++) {
        val2.push_back( convert_voltage((Short_t)c2[i], ydiv_c2, voffset_c2) );
        }

        //for C3
        osc.SendMessage("C3:WF? DAT2");//or ALL
        osc.ReceiveMessage();
        char *c3 = osc.getBuffer();
        for (int i =0 ; i< datasize ; i++) {
        val3.push_back( convert_voltage((Short_t)c3[i], ydiv_c3, voffset_c3) ) ;
        }

        //for C4
        osc.SendMessage("C4:WF? DAT2");//or ALL
        osc.ReceiveMessage();
        char *c4 = osc.getBuffer();
        for (int i =0 ; i< datasize ; i++) {
        val4.push_back( convert_voltage( (Short_t)c4[i], ydiv_c4, voffset_c4) );
        }

        cout<< "Stoping t4:" << endl;
        time_t transfer_end_Sec;
        Int_t transfer_end_NanoSec;
        time_Temp_Set_Again->Set();
        transfer_end_Sec = time_Temp_Set_Again->GetSec()- time_Zone_Offset; // Second from 1970 to now
        transfer_end_NanoSec = time_Temp_Set_Again->GetNanoSec(); // nano second from now
        TTimeStamp transfer_end_Convertion(transfer_end_Sec, transfer_end_NanoSec ); // TTimeStamp (time_t t, Int_t nsec)
        transfer_end_Convertion_Sec_And_NanoSec = transfer_end_Convertion.AsDouble();
        time_Transfer_Data->Fill( transfer_end_Convertion_Sec_And_NanoSec-transfer_Start_Time_Convertion_Sec_And_NanoSec );
        // cout <<" transfer_end_Convertion: " << transfer_end_Convertion << endl;


        processing_class processing_ch1;
        processing_class processing_ch2;
        processing_class processing_ch3;
        processing_class processing_ch4;
        
        Float_t threshold = -0.010;
        Float_t threshold3 = -0.020;
        // calling the constructor in steps
        processing_ch1.get_Data(val1);
        processing_ch2.get_Data(val2);
        processing_ch3.get_Data(val3);
        processing_ch4.get_Data(val4);
        
        
        cout<< "Stoping t4.1:" << endl;
        // If you want to use smoothed data, let's remove comment
        // processing_ch1.smooth_Data(3);
        // processing_ch2.smooth_Data(3);
        // processing_ch3.smooth_Data(3);
        // processing_ch4.smooth_Data(3);


        // for(int i =0 ; processing_ch1.smoothed.size();i++) {cout << processing_ch1.smoothed[i] << endl;}
        // If you want to use smoothed data, let's alternate processing_ch1.data to processing_ch1.smoothed
        cout<< "Stoping t4.2:" << endl;
        processing_ch1.find_Minima(processing_ch1.data, threshold);
        processing_ch2.find_Minima(processing_ch2.data,threshold);
        processing_ch3.find_Minima(processing_ch3.data,threshold3);
        processing_ch4.find_Minima(processing_ch4.data,threshold);

        cout<< "Stoping t4.3:" << endl;
        processing_ch1.reduction(40,100,tdiv, samplingrate);
        processing_ch2.reduction(40,100, tdiv, samplingrate);
        processing_ch3.reduction(40,100, tdiv, samplingrate);
        processing_ch4.reduction(40,100, tdiv, samplingrate);


        //vector<int> minima1 = processing_ch1.minima;
        //vector<Float_t> value_reduction1 = processing_ch1.reduction;
        //vector<float> value_reduction2 = processing_ch2.reduction;
        // for(int i =0 ; processing_ch3.smoothed.size();i++) {cout << processing_ch3.smoothed[i] << endl;}
        //vector<float> value_reduction3 = processing_ch3.reduction;
        //vector<float> value_reduction4 = processing_ch4.reduction;

        
        cout<< "Stoping t4.4:" << endl;
        //for (char *c = osc.getBuffer()+21; c <= osc.getBuffer()+19 + datasize; c += 2) { //2.5E+8 Sampling per second <=> 4 ns separation 
        //5E+8 Sampling per second <=> 2 ns separation
        for (int i = 0 ; i< processing_ch1.data_Reduction.size() ; i++){ 
        pwfgraph_c1->SetPoint(i, processing_ch1.time_Reduction[i], processing_ch1.data_Reduction[i]);
        }

        for (int i = 0 ; i< processing_ch2.data_Reduction.size() ; i++){ 
        pwfgraph_c2->SetPoint(i, processing_ch2.time_Reduction[i], processing_ch2.data_Reduction[i]);
        }

        for (int i = 0 ; i< processing_ch3.data_Reduction.size() ; i++){ 
        pwfgraph_c3->SetPoint(i, processing_ch3.time_Reduction[i], processing_ch3.data_Reduction[i]);
        }

        for (int i = 0 ; i< processing_ch4.data_Reduction.size() ; i++){ 
        pwfgraph_c4->SetPoint(i, processing_ch4.time_Reduction[i], processing_ch4.data_Reduction[i]);
        }
        


        cout<< "Stoping t5:" << endl;
        time_t convertVolt_End_Sec;
        Int_t convertVolt_End_NanoSec;
        time_Temp_Set_Again->Set();
        convertVolt_End_Sec = time_Temp_Set_Again->GetSec()- time_Zone_Offset; // Second from 1970 to now
        convertVolt_End_NanoSec = time_Temp_Set_Again->GetNanoSec(); // nano second from now
        TTimeStamp convertVolt_End_Convertion(convertVolt_End_Sec, convertVolt_End_NanoSec ); // TTimeStamp (time_t t, Int_t nsec)
        convertVolt_End_Convertion_Sec_And_NanoSec = convertVolt_End_Convertion.AsDouble();
        time_Convert_Data->Fill(convertVolt_End_Convertion_Sec_And_NanoSec - transfer_end_Convertion_Sec_And_NanoSec );
        cout <<" convertVolt_End_Convertion: " << convertVolt_End_Convertion << endl;

        outFile->cd();
        pwfgraph_c1->Write(Form("wf_ch1_%d",i_frame));
        pwfgraph_c2->Write(Form("wf_ch2_%d",i_frame));
        pwfgraph_c3->Write(Form("wf_ch3_%d",i_frame));
        pwfgraph_c4->Write(Form("wf_ch4_%d",i_frame));
        dataTree->Fill();
        dataTree->AutoSave("SaveSelf");

        processing_ch1.clearVector();
        processing_ch2.clearVector();
        processing_ch3.clearVector();
        processing_ch4.clearVector();

        delete pwfgraph_c1;
        delete pwfgraph_c2;
        delete pwfgraph_c3;
        delete pwfgraph_c4;

    }
    frame_time.clear();
    time_Dif_Frame->Write(); 
    time_Transfer_Data->Write();
    time_Convert_Data->Write(); 
    dataTree->Write();
    outFile->Close();
    
    osc.Close();
    cout << "Finish the RUN" << endl;
   
    //delete dataTree;
    //delete outFile;
    return 0;
}
