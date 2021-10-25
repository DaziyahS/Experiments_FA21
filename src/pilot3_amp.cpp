#include <Mahi/Gui.hpp>
#include <Mahi/Util.hpp>
#include <Mahi/Util/Logging/Log.hpp>
#include <syntacts>
#include <random>
#include <iostream> 
#include <fstream> // need to include inorder to save to csv
#include <chrono> 
#include <string> // for manipulating file name

// local includes
#include <Chord.hpp>
#include <Note.hpp> // would this imply that the .cpp functionality is attached?
// #include <notes_info.hpp> // probably do not need

// open the namespaces that are relevant for this code
using namespace mahi::gui;
using namespace mahi::util;
using tact::Signal;
using tact::sleep;
using tact::Sequence;

// deteremine application variables
int windowWidth = 1800; // 1920 x 1080 is screen dimensions
int windowHeight = 1000;
std::string my_title= "Play GUI";
ImVec2 buttonSize = ImVec2(400, 65);  // Size of buttons on GUI
// std::string deviceNdx = "Speakers (USB Sound Device)"; // Put my device name or number, is for at home name
int deviceNdx = 6;
// tactors of interest
int topTact = 4;
int botTact = 6;
int leftTact = 0;
int rightTact = 2;

// trying to figure out how to save to an excel document
std::string saveSubject; // experiment details, allows me to customize
std::ofstream file_name; // this holds the trial information

class MyGui : public Application
{
    // Start by declaring the session variable
    tact::Session s; // this ensures the whole app knows this session
public:
    // this is a constructor. It initializes your class to a specific state
    MyGui() : 
    Application(windowWidth, windowHeight, my_title, 0),
    chordNew(),
    channelSignals(3)
    {
        s.open(deviceNdx); // , tact::API::MME); // opens session with the application
        // keep in mind, if use device name must also use the API
        // something the GUI needs *shrugs*
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        set_background(Cyans::Teal); //background_color = Grays::Black; 

        // so the current chord can play immediately
        currentChord = chordNew.signal_list[14];
     }

    // Define variables needed throughout the program
    // For creating the signal
    std::string currentChord; // holds name of current chord based on selection
    Chord chordNew;
    std::vector<tact::Signal> channelSignals;
    bool isSim = false; // default is sequential
    // For saving the signal
    std::string sigName; // name for saved signal
    std::string fileLocal; // for storing the signal
    // For saving records
    int trial_num = 0;
    int val = 0, arous = 0;
    int final_trial_num = 5;
    // For playing the signal
    Clock play_clock; // keeping track of time for non-blocking pauses
    bool play_once = false;    // for playing a cue one time    
    // Set up timing within the trials itself
    Clock trial_clock;
    Time time2;
    // The amplitudes in a vector
    std::vector<int> list = {0, 1, 2, 3};

    bool first_in_trial = false;
    // for screens
    std::string screen_name = "trial_screen";
    int exp_num = 1;

    virtual void update() override
    {
        ImGui::Begin("Playing GUI");
        ImGui::Text("The current experiment number is: %i", exp_num);
        
        if (screen_name == "begin_screen")
        {
            beginScreen();
        }
        else if (screen_name == "trans_screen")
        {
            transScreen();
            first_in_trial = true;
        }
        else if (screen_name == "trial_screen")
        {
            trialScreen();
        }
        else if (screen_name == "end_screen")
        {
            endScreen();
        }
        
        ImGui::End();

    }


/*
// beginning screen
Subject Number Prompt
Input Subject Number
Press submit button
    store the subject number
    create an excel file based on the subject number
*/
void beginScreen()
{
    // in case I can possibly make a new file this way!
    if(ImGui::Button("Subject Number", buttonSize))
    {
        ImGui::OpenPopup("subject_num"); // open a popup and name it for calling
        // This just needs its own space, no curlies for the if
    }  
    static char name[12]; // info holder 
    // take the subjects number
    if(ImGui::BeginPopup("subject_num")) // if clicked essentially
    {
        ImGui::Text("What is the subject's number: "); // precursor for me to understand
        ImGui::InputText("##edit", name, IM_ARRAYSIZE(name)); // size wanted           
        if (ImGui::Button("Close"))
        {
            ImGui::CloseCurrentPopup();
            // put things here for what should happen once closed or else it will run foreverrrr 
            
            // Declare value for saveSubject everywhere
            saveSubject = name;
            // Create a new file 
            file_name.open("../../Data/" + saveSubject + "_pilotingAmp.csv"); // saves the csv name for all parameters

            // Go to next screen
            screen_name = "trans_screen";
        }
        ImGui::EndPopup();
    }
}

/*
// transition screen
update trial number
make sure the excel file is being created for the following trials
    create new name for new excel file based on subject number and trial number
write the first line of the excel file
tell the person to talk to me, or give them more information
*/
void transScreen()
{
    // update trial number
    trial_num++;
    char trial_numChar = (char)(trial_num + 48); // adds 0 char value for us

    // Create a new file 
    file_name.open("../../Data/" + saveSubject + "_pilotingAmp_" + trial_numChar + ".csv"); // saves the csv name for all parameters
    // First line of the code
    file_name << "Trial" << "," << "Chord" << "," << "Sus" << "," << "Amp" << "," << "IsSim" << "," << "IsMajor" << ","
              << "Valence" << "," << "Arousal" << "," << "Notes" << std::endl; // theoretically setting up headers
    // Write message for person
    ImGui::Text("Trial number is your intermediate screen.");

    // Go to next screen
    screen_name = "trial_screen";
}

/*
// trial 1
include information for timestamp
set up the parameters
    define the base cue parameters
randomize the amplitudes wanted into a vector
    math to determine trials: 10 minute session, 15 seconds to choose, 600 s / 15 s = 40 trials
    create vector of length 40 with 10 of each amplitude option
    randomize the vector
display
    display each of the SAMs with their numbers
    have a drop down for person to choose which OR have them have buttons underneath them (must hold value chose visibly)
        updates the valence and arousal values
press button
    if not at end of vector
        record the data
        reset the values of the valence and arousal display
        increase the trial iterator
    else if end of vector && not at max trial
        record the data
        go to transition screen
        reset the trial iterator
        increase the experiment number
    else // aka at max trial number
        record the data
        go to end of experiment screen
*/
void trialScreen()
{
    // Set up the paramaters
    // Define the base cue paramaters
    sus = 1;
    isSim = false; // sequential
    // for (size_t i = 0; i < 10; i++)
    // {
    //     for (size_t j = 0; j < 4; j++)
    //     {
    //         list.push_back(j);
    //     }    
    // }

    // internal trial tracker
    static int count = 0;
    // random number generator
    static auto rng = std::default_random_engine {};

    if (first_in_trial){
        // initial randomization
        std::shuffle(std::begin(list), std::end(list), rng);
        // counter for trial starts at 0 in beginning
        count = 0;
        // set first_in_trial to false so initial randomization can happen once
        first_in_trial = false;
    }
    
    if (count < 40){
        // Play the cue
        if(ImGui::Button("Play")){
            int cue_num = count%4;
            play_trial(cue_num);

        }
        // Go to next cue
        if(ImGui::Button("Next")){
            // Record the answers

            // shuffle the list if needed
            int cue_num = count % 4;
            if (cue_num == 3){
                std::shuffle(std::begin(list), std::end(list), rng);            
            }
            // increase the list number
            count++;
        }
    }
    else // if trials are done
    {
        if(trial_num < final_trial_num) // if not final trial
        {
            screen_name = "trans_screen";
            file_name.close();
        }
        else // if final trial
        {
            screen_name = "end_screen";
            file_name.close();
        }
    }    
}

/*
// end of experiment screen
blank?
All done. Thank you for your participation
*/
void endScreen()
{
    ImGui::Text("Thank you for your participation!");
}

}

/*

Let's do a fake code

// beginning screen
Subject Number Prompt
Input Subject Number
Press submit button
    store the subject number
    create an excel file based on the subject number

// transition screen
update trial number
make sure the excel file is being created for the following trials
    create new name for new excel file based on subject number and trial number
write the first line of the excel file
tell the person to talk to me, or give them more information

// trial 1
include information for timestamp
set up the parameters
    define the base cue parameters
randomize the amplitudes wanted into a vector
    math to determine trials: 10 minute session, 15 seconds to choose, 600 s / 15 s = 40 trials
    create vector of length 40 with 10 of each amplitude option
    randomize the vector
display
    display each of the SAMs with their numbers
    have a drop down for person to choose which OR have them have buttons underneath them (must hold value chose visibly)
        updates the valence and arousal values
press button
    if not at end of vector
        record the data
        reset the values of the valence and arousal display
        increase the trial iterator
    else if end of vector && not at max trial
        record the data
        go to transition screen
        reset the trial iterator
        increase the experiment number
    else // aka at max trial number
        record the data
        go to end of experiment screen

// end of experiment screen
blank?
All done. Thank you for your participation

*/