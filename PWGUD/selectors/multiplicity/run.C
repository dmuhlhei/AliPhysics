void run(Char_t* data, Long64_t nRuns = -1, Long64_t offset = 0, Bool_t aDebug = kFALSE, Int_t aProof = 0, Int_t requiredData = 1, const char* option = "",Int_t workers = -1)
{
  // aProof option: 
  //               -2 grid, terminate mode
  //               -1 grid, full mode
  //                0 local
  //                1 proof with chain
  //                2 proof with dataset
  //                3 local with proof dataset 
  //
  // requiredData option: 0 = only ESD
  //                      1 = ESD+MC
  //                      2 = RAW (ESD+check on event type)
  //

  if (nRuns < 0)
    nRuns = 1234567890;

  if (aProof > 0)
  {
    gEnv->SetValue("XSec.GSI.DelegProxy", "2");
    TProof::Open("alice-caf.cern.ch", workers>0 ? Form("workers=%d",workers) : "");
    
    // Enable the needed package
    if (1)
    {
      gProof->UploadPackage("$ALICE_ROOT/STEERBase");
      gProof->EnablePackage("$ALICE_ROOT/STEERBase");
      gProof->UploadPackage("$ALICE_ROOT/ESD");
      gProof->EnablePackage("$ALICE_ROOT/ESD");
      gProof->UploadPackage("$ALICE_ROOT/AOD");
      gProof->EnablePackage("$ALICE_ROOT/AOD");
      gProof->UploadPackage("$ALICE_ROOT/ANALYSIS");
      gProof->EnablePackage("$ALICE_ROOT/ANALYSIS");
      gProof->UploadPackage("$ALICE_ROOT/ANALYSISalice");
      gProof->EnablePackage("$ALICE_ROOT/ANALYSISalice");
    }
    else
    {
      gProof->UploadPackage("$ALICE_ROOT/AF-v4-16");
      gProof->EnablePackage("$ALICE_ROOT/AF-v4-16");
    }

    gProof->UploadPackage("$ALICE_ROOT/PWG0base");
    gProof->EnablePackage("$ALICE_ROOT/PWG0base");
  }
  else
  {
    gSystem->Load("libVMC");
    gSystem->Load("libTree");
    gSystem->Load("libSTEERBase");
    gSystem->Load("libESD");
    gSystem->Load("libAOD");
    gSystem->Load("libANALYSIS");
    gSystem->Load("libANALYSISalice");
    gSystem->Load("libPWG0base");
    
    gROOT->ProcessLine(gSystem->ExpandPathName(".include $ALICE_ROOT/PWG0"));
  }

  // Create the analysis manager
  mgr = new AliAnalysisManager;

  // Add ESD handler
  AliESDInputHandler* esdH = new AliESDInputHandler;
  esdH->SetReadFriends(kFALSE);
  //esdH->SetInactiveBranches("AliESDACORDE FMD ALIESDTZERO ALIESDZDC AliRawDataErrorLogs CaloClusters Cascades EMCALCells EMCALTrigger ESDfriend Kinks AliESDTZERO ALIESDACORDE MuonTracks TrdTracks");
  mgr->SetInputEventHandler(esdH);

  // physics selection
  gROOT->ProcessLine(".L $ALICE_ROOT/OADB/macros/AddTaskPhysicsSelection.C");
  physicsSelectionTask = AddTaskPhysicsSelection((requiredData == 2) ? kFALSE : kTRUE);

  // FO efficiency (for MC)
  if (0 && requiredData != 2)
  {
    //const char* fastORFile = "../dNdEta/spdFOEff_run104824_52.root";
    //const char* fastORFile = "../dNdEta/spdFOEff_run104867_92.root";
    //const char* fastORFile = "../dNdEta/spdFOEff_run105054_7.root";
    const char* fastORFile = "../dNdEta/spdFOEff_run114931.root";
  
    Printf("NOTE: Simulating FAST-OR efficiency on the analysis level using file %s", fastORFile);
    TFile::Open(fastORFile);
    
    spdFOEff = (TH1F*) gFile->Get("spdFOEff");
    physicsSelectionTask->GetPhysicsSelection()->Initialize(114931);
    physicsSelectionTask->GetPhysicsSelection()->GetTriggerAnalysis()->SetSPDGFOEfficiency(spdFOEff);
  }
  
  //AliPWG0Helper::AnalysisMode analysisMode = AliPWG0Helper::kSPD | AliPWG0Helper::kFieldOn;
  AliPWG0Helper::AnalysisMode analysisMode = AliPWG0Helper::kTPCSPD | AliPWG0Helper::kFieldOn;
  //AliPWG0Helper::AnalysisMode analysisMode = AliPWG0Helper::kTPCITS | AliPWG0Helper::kFieldOn;
  
  AliTriggerAnalysis::Trigger trigger      = AliTriggerAnalysis::kAcceptAll | AliTriggerAnalysis::kOfflineFlag;
  //AliTriggerAnalysis::Trigger trigger      = AliTriggerAnalysis::kAcceptAll | AliTriggerAnalysis::kOfflineFlag | AliTriggerAnalysis::kOneParticle; 
  
  //AliTriggerAnalysis::Trigger trigger      = AliTriggerAnalysis::kMB1Prime | AliTriggerAnalysis::kOfflineFlag;
  //AliTriggerAnalysis::Trigger trigger      = AliTriggerAnalysis::kSPDGFOBits | AliTriggerAnalysis::kOfflineFlag;
  //AliTriggerAnalysis::Trigger trigger      = AliTriggerAnalysis::kV0AND | AliTriggerAnalysis::kOfflineFlag; 

  AliPWG0Helper::DiffTreatment diffTreatment = AliPWG0Helper::kMCFlags;
  //AliPWG0Helper::DiffTreatment diffTreatment = AliPWG0Helper::kE710Cuts;
  
  AliPWG0Helper::PrintConf(analysisMode, trigger, diffTreatment);

  TString taskName("AliMultiplicityTask.cxx+");
  if (aDebug)
    taskName += "+g";

  // Create, add task
  if (aProof > 0) {
    gProof->Load(taskName);
  } else
    gROOT->Macro(taskName);

  // 0 bin calculation
  if (0)
  {
  }
  
  // V0 syst. study
  if (0)
  {
    Printf("NOTE: Systematic study for VZERO enabled!");
    //physicsSelectionTask->GetPhysicsSelection()->Initialize(104867);
    for (Int_t i=0; i<1; i++)
    {
      // for MC and data
      physicsSelectionTask->GetPhysicsSelection()->GetTriggerAnalysis(i)->SetV0HwPars(15, 61.5, 86.5);
      physicsSelectionTask->GetPhysicsSelection()->GetTriggerAnalysis(i)->SetV0AdcThr(15);
      // only for MC
      //physicsSelectionTask->GetPhysicsSelection()->GetTriggerAnalysis(i)->SetV0HwPars(0, 0, 125);
      //physicsSelectionTask->GetPhysicsSelection()->GetTriggerAnalysis(i)->SetV0AdcThr(0);
    }
  }

  TString optionStr(option);
  
  // remove SAVE option if set
  Bool_t save = kFALSE;
  TString optionStr(option);
  if (optionStr.Contains("SAVE"))
  {
    optionStr = optionStr(0,optionStr.Index("SAVE")) + optionStr(optionStr.Index("SAVE")+4, optionStr.Length());
    save = kTRUE;
  }
  
  task = new AliMultiplicityTask(optionStr);

  if (!(analysisMode & AliPWG0Helper::kSPD))
  {
    // selection of esd tracks
    gROOT->ProcessLine(".L $ALICE_ROOT/PWG0/CreateStandardCuts.C");
    AliESDtrackCuts* esdTrackCuts = CreateTrackCuts(analysisMode);
    if (!esdTrackCuts)
    {
      printf("ERROR: esdTrackCuts could not be created\n");
      return;
    }

    task->SetTrackCuts(esdTrackCuts);
  }
  //else
  //  task->SetDeltaPhiCut(0.05);

  task->SetAnalysisMode(analysisMode);
  task->SetTrigger(trigger);
  task->SetDiffTreatment(diffTreatment);

  if (requiredData == 1)
    task->SetReadMC();

  //task->SetUseMCVertex();
  
  //if (requiredData != 2)
  //  task->SetSkipParticles();

  mgr->AddTask(task);

  if (requiredData == 1) {
    // Enable MC event handler
    AliMCEventHandler* handler = new AliMCEventHandler;
    if (!optionStr.Contains("particle-efficiency"))
      handler->SetReadTR(kFALSE);
    mgr->SetMCtruthEventHandler(handler);
  }

  // pt study
  if (optionStr.Contains("pt-spectrum-func"))
  {
    TF1* func = new TF1("func", "1", 0, 0.2);
    //TF1* func = new TF1("func", "1.5 - x / 0.2 * 0.5", 0, 0.2);
    //TF1* func = new TF1("func", "1.25 - x / 0.2 * 0.25", 0, 0.2);
    //TF1* func = new TF1("func", "0.75 + x / 0.2 * 0.25", 0, 0.2);
    hist = func->GetHistogram();
    //new TCanvas; func->Draw();
    //inputList.Add(func->GetHistogram()->Clone("pt-spectrum"));

    new TCanvas; hist->Draw();
    task->SetPtSpectrum((TH1D*) hist->Clone("pt-spectrum"));
  }

  // Attach input
  cInput  = mgr->GetCommonInputContainer();
  mgr->ConnectInput(task, 0, cInput);

  // Attach output
  cOutput = mgr->CreateContainer("cOutput", TList::Class(), AliAnalysisManager::kOutputContainer, "mult_Temp.root");
  mgr->ConnectOutput(task, 0, cOutput);

  // Enable debug printouts
  if (aDebug)
    mgr->SetDebugLevel(2);

  // Run analysis
  mgr->InitAnalysis();
  mgr->PrintStatus();

  if (aProof == 2)
  {
    // process dataset

    mgr->StartAnalysis("proof", data, nRuns, offset);
  
    if (save)
    {
      TString path("maps/");
      path += TString(data).Tokenize("/")->Last()->GetName();
      
      UInt_t triggerNoFlags = (UInt_t) trigger % (UInt_t) AliTriggerAnalysis::kStartOfFlags;
      switch (triggerNoFlags)
      {
        case AliTriggerAnalysis::kAcceptAll: path += "/all"; break;
        case AliTriggerAnalysis::kMB1: path += "/mb1"; break;
        case AliTriggerAnalysis::kMB2: path += "/mb2"; break;
        case AliTriggerAnalysis::kMB3: path += "/mb3"; break;
        case AliTriggerAnalysis::kSPDGFO: path += "/spdgfo"; break;
        case AliTriggerAnalysis::kSPDGFOBits: path += "/spdgfobits"; break;
        case AliTriggerAnalysis::kV0AND: path += "/v0and"; break;
        case AliTriggerAnalysis::kNSD1: path += "/nsd1"; break;
        case AliTriggerAnalysis::kMB1Prime: path += "/mb1prime"; break;
        default: Printf("ERROR: Trigger undefined for path to files"); return;
      }
      
      if (trigger & AliTriggerAnalysis::kOneParticle)
        path += "-onepart";
      
      if (analysisMode & AliPWG0Helper::kSPD)
        path += "/spd";

      if (analysisMode & AliPWG0Helper::kTPCSPD)
        path += "/tpcspd";
      
      if (analysisMode & AliPWG0Helper::kTPC)
        path += "/tpc";
        
      if (analysisMode & AliPWG0Helper::kTPCITS)
        path += "/tpcits";

      gSystem->mkdir(path, kTRUE);
      
      TString fileName("multiplicity");
      if (optionStr.Contains("only-process-type-nd"))
        fileName += "ND";
      if (optionStr.Contains("only-process-type-sd"))
        fileName += "SD";
      if (optionStr.Contains("only-process-type-dd"))
        fileName += "DD";
      fileName += ".root";
      
      gSystem->Rename(fileName, path + "/" + fileName);
      gSystem->Rename("event_stat.root", path + "/event_stat.root");
      
      Printf(">>>>> Moved files to %s", path.Data());
    }  
  }
  else if (aProof == 3)
  {
    gROOT->ProcessLine(".L CreateChainFromDataSet.C");
    ds = gProof->GetDataSet(data)->GetStagedSubset();
    chain = CreateChainFromDataSet(ds, "esdTree", nRuns);
    mgr->StartAnalysis("local", chain, nRuns, offset);
  }
  else if (aProof == 0)
  {
    // Create chain of input files
    gROOT->LoadMacro("$ALICE_ROOT/PWG0/CreateESDChain.C");
    chain = CreateESDChain(data, nRuns, offset);

    mgr->StartAnalysis((aProof > 0) ? "proof" : "local", chain);
  }
  else if (aProof == -1 || aProof == -2)
  {
    gSystem->Unlink("EventStat_temp.root");
    gSystem->Unlink("mult_Temp.root");
    gSystem->Unlink("event_stat.root");
    gSystem->Unlink("multiplicity.root");
  
    AliAnalysisAlien *plugin = new AliAnalysisAlien();
  
    if (aProof == -1)
      plugin->SetRunMode("full");
    else
      plugin->SetRunMode("terminate");

    plugin->SetAPIVersion("V1.1x");
    plugin->SetROOTVersion("v5-26-00b-2");
    //plugin->SetAliROOTVersion("v4-19-10-AN");
  
    //plugin->SetGridDataDir("/alice/sim/LHC10b6");
    //plugin->SetDataPattern("*/AliESDs.root");
    //plugin->SetRunRange(115186, 115210);
    
    //plugin->SetGridDataDir("/alice/data/2010/LHC10b");
    //plugin->SetDataPattern("*/AliESDs.root");
    //plugin->SetRunRange(114931, 114931);
    //plugin->SetRunPrefix("000");
    
  //plugin->AddDataFile("/alice/cern.ch/user/j/jgrosseo/run114931_6.xml"); //LHC10b4
  
    TString dataStr(data);
    dataStr.ReplaceAll("__", "#");
    TObjArray* dataList = dataStr.Tokenize("#");
    for (Int_t i=0; i<dataList->GetEntries(); i++)
      plugin->AddDataFile(Form("/alice/cern.ch/user/j/jgrosseo/%s", dataList->At(i)->GetName()));
  
    TGrid::Connect("alien://");
    
    // use work dir from ds name
    TString dir;
    dir.Form("gridjob/dir_%s", data);
    if (aProof == -1)
      gGrid->Rmdir(dir);
    
    /*
    Int_t n = 0;
    while (n < 1000)
    {
      dir.Form("gridjob/dir%d", n);
      TMap* mapLs = (TMap*) gGrid->Ls(dir)->At(0);
      TObjString *valueLsPath = dynamic_cast<TObjString*>(mapLs->GetValue("path"));
      if (!valueLsPath)
        break;
      n++;
    }
    if (n == 1000)
      AliFatal("Clean your home dir!");
    if (aProof == -2)
      n--;
    dir.Form("gridjob/dir%d", n);
    */
    Printf(">>>>> Using relative dir %s", dir.Data());
  
    plugin->SetGridWorkingDir(dir);
    plugin->SetGridOutputDir("griddir");
    plugin->SetDefaultOutputs();
    
    //
    plugin->SetFastReadOption();
  
  //plugin->EnablePackage(gSystem->ExpandPathName("$ALICE_ROOT/PWG0base.par"));
    plugin->EnablePackage(gSystem->ExpandPathName("STEERBase.par"));
    plugin->EnablePackage(gSystem->ExpandPathName("ESD.par"));
    plugin->EnablePackage(gSystem->ExpandPathName("AOD.par"));
    plugin->EnablePackage(gSystem->ExpandPathName("ANALYSIS.par"));
    plugin->EnablePackage(gSystem->ExpandPathName("ANALYSISalice.par"));
    plugin->EnablePackage(gSystem->ExpandPathName("PWG0base.par"));
    plugin->SetAnalysisSource("AliMultiplicityTask.cxx");
    plugin->SetAdditionalLibs("AliMultiplicityTask.h AliMultiplicityTask.cxx");
  
    plugin->SetSplitMaxInputFileNumber(50);
    plugin->SetNtestFiles(2);
  
    AliAnalysisManager::GetAnalysisManager()->SetGridHandler(plugin);
    
    mgr->StartAnalysis("grid");
    if (aProof == -1)
      return;
  }

  if (save)
  {
    TString path("maps/");
    path += TString(data).Tokenize("/")->Last()->GetName();
  
    UInt_t triggerNoFlags = (UInt_t) trigger % (UInt_t) AliTriggerAnalysis::kStartOfFlags;
    switch (triggerNoFlags)
    {
      case AliTriggerAnalysis::kAcceptAll: path += "/all"; break;
      case AliTriggerAnalysis::kMB1: path += "/mb1"; break;
      case AliTriggerAnalysis::kMB2: path += "/mb2"; break;
      case AliTriggerAnalysis::kMB3: path += "/mb3"; break;
      case AliTriggerAnalysis::kSPDGFO: path += "/spdgfo"; break;
      case AliTriggerAnalysis::kSPDGFOBits: path += "/spdgfobits"; break;
      case AliTriggerAnalysis::kV0AND: path += "/v0and"; break;
      case AliTriggerAnalysis::kNSD1: path += "/nsd1"; break;
      case AliTriggerAnalysis::kMB1Prime: path += "/mb1prime"; break;
      default: Printf("ERROR: Trigger undefined for path to files"); return;
    }
  
    if (trigger & AliTriggerAnalysis::kOneParticle)
      path += "-onepart";
  
    if (analysisMode & AliPWG0Helper::kSPD)
      path += "/spd";
  
    if (analysisMode & AliPWG0Helper::kTPC)
      path += "/tpc";
    
    if (analysisMode & AliPWG0Helper::kTPCITS)
      path += "/tpcits";

    gSystem->mkdir(path, kTRUE);
  
    TString fileName("multiplicity");
    if (optionStr.Contains("only-process-type-nd"))
      fileName += "ND";
    if (optionStr.Contains("only-process-type-sd"))
      fileName += "SD";
    if (optionStr.Contains("only-process-type-dd"))
      fileName += "DD";
    fileName += ".root";
  
    gSystem->Rename(fileName, path + "/" + fileName);
    gSystem->Rename("event_stat.root", path + "/event_stat.root");
  
    Printf(">>>>> Moved files to %s", path.Data());
  }
}
