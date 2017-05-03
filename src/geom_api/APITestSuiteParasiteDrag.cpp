//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "VSP_Geom_API.h"
#include "APITestSuiteParasiteDrag.h"
#include <float.h>

//Default tolerance to use for tests.  Most calculations are done as doubles and choosing single precision FLT_MIN gives some allowance for precision stackup in calculations
#define TEST_TOL FLT_MIN

//=============================================================================//
//======================== APITestSuiteParasiteDrag ===========================//
//=============================================================================//

void APITestSuiteParasiteDrag::TestParasiteDragCreateModel()
{
    printf("APITestSuiteParasiteDrag::TestParasiteDragCreateModel()\n");

    // make sure setup works
    vsp::VSPCheckSetup();
    vsp::VSPRenew();
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

                                                                 //==== Add Wing Geom and set some parameters =====//
    printf("\tAdding WING (Main)\n");
    string wing_id = vsp::AddGeom( "WING");
    vsp::SetGeomName( wing_id, "MainWing" );
    TEST_ASSERT( wing_id.c_str() != NULL );
    TEST_ASSERT_DELTA( vsp::SetParmValUpdate(  wing_id, "TotalSpan", "WingGeom", 17.0 ), 17.0, TEST_TOL);
    TEST_ASSERT_DELTA( vsp::SetParmValUpdate(  wing_id, "Z_Rel_Location", "XForm", 0.5 ), 0.5, TEST_TOL);

    printf("\tAdding POD\n");
    string pod_id = vsp::AddGeom( "POD");
    TEST_ASSERT( pod_id.c_str() != NULL );
    TEST_ASSERT_DELTA( vsp::SetParmValUpdate(  pod_id, "Length", "Design", 14.5 ), 14.5, TEST_TOL);
    TEST_ASSERT_DELTA( vsp::SetParmValUpdate(  pod_id, "X_Rel_Location", "XForm", -3.0 ), -3.0, TEST_TOL);

    vsp::Update();
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE
                                                                 //==== Setup export filenames ====//
                                                                 // Execution of one of these methods is required to propperly set the export filenames for creation of vspaero input files and execution commands
    m_vspfname_for_parasitedragtests = "apitest_TestParasiteDrag.vsp3";
    printf("\tSetting export name: %s\n", m_vspfname_for_parasitedragtests.c_str());
    vsp::SetVSP3FileName( m_vspfname_for_parasitedragtests );  // this still needs to be done even if a call to WriteVSPFile is made
    vsp::Update();
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

                                                                 //==== Save Vehicle to File ====//
    printf("\tSaving vehicle file to: %s ...\n", m_vspfname_for_parasitedragtests.c_str());
    vsp::WriteVSPFile( vsp::GetVSPFileName(), vsp::SET_ALL );
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE
    printf("COMPLETE\n");

    // Final check for errors
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE
    printf("\n");
}

void APITestSuiteParasiteDrag::TestFirstParasiteDragCalc()
{
    double pd_tol = 0.00001;
    double first_res = 0.00387;

    double geom_tol = 0.01;
    double wing_swet = 74.96;
    double pod_swet = 60.81;
    double wing_lref = 3.09;
    double pod_lref = 14.50;

    // make sure setup works
    vsp::VSPCheckSetup();
    vsp::VSPRenew();
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

                                                                 //open the file created in TestVSPAeroCreateModel
    vsp::ReadVSPFile( m_vspfname_for_parasitedragtests );
    if ( m_vspfname_for_parasitedragtests == string() )
    {
        TEST_FAIL("m_vspfname_for_parasitedragtests = NULL, need to run: APITestSuite::TestFirstParasiteDragCalc");
        return;
    }
    if ( vsp::ErrorMgr.PopErrorAndPrint( stdout ) )
    {
        TEST_FAIL("m_vspfname_for_parasitedragtests failed to open");
        return;
    }

    // Execute
    printf("\tExecuting...\n");
    string results_id = vsp::ExecAnalysis("ParasiteDrag");
    printf("COMPLETE\n");
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

    vector < double > swet = vsp::GetDoubleResults( results_id, "Comp_Swet" );
    TEST_ASSERT_DELTA( wing_swet, swet[0], geom_tol );
    TEST_ASSERT_DELTA( pod_swet, swet[2], geom_tol );

    vector < double > lref = vsp::GetDoubleResults( results_id, "Comp_Lref" );
    TEST_ASSERT_DELTA( wing_lref, lref[0], geom_tol );
    TEST_ASSERT_DELTA( pod_lref, lref[2], geom_tol );

    vector < double > dat = vsp::GetDoubleResults( results_id, "Total_Cd_Total" );
    TEST_ASSERT_DELTA( first_res, dat[0], pd_tol );

    vsp::PrintResults( results_id );                             // Get & Display Results

                                                                 // Final check for errors
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE
    printf("\n");
}

void APITestSuiteParasiteDrag::TestAddExcrescence()
{
    double excres_inputs[] = { 2.0, 0.0001, 5.0, 5.0, 0.03 };
    double excres_amounts[] = { 0.0002, 0.0001, 0.00019, 0.00025, 0.00030 };
    double excres_tol = 0.0001;
    size_t nexcres = 5;

    // Add First Excrescence
    vsp::AddExcrescence( "Count", vsp::EXCRESCENCE_COUNT, excres_inputs[0] );
    string results_id = vsp::ExecAnalysis("ParasiteDrag");
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

    vector < int > dat_int = vsp::GetIntResults( results_id, "Num_Excres" );
    TEST_ASSERT( dat_int[0] == 1 );

    // Add Second Excrescence
    vsp::AddExcrescence( "Cd", vsp::EXCRESCENCE_CD, excres_inputs[1] );
    results_id = vsp::ExecAnalysis("ParasiteDrag");
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

    dat_int = vsp::GetIntResults( results_id, "Num_Excres" );
    TEST_ASSERT( dat_int[0] == 2 );

    // Add Third Excrescence
    vsp::AddExcrescence( "Percentage", vsp::EXCRESCENCE_PERCENT_GEOM, excres_inputs[2] );
    results_id = vsp::ExecAnalysis("ParasiteDrag");
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

    dat_int = vsp::GetIntResults( results_id, "Num_Excres" );
    TEST_ASSERT( dat_int[0] == 3 );

    // Add Fourth Excrescence
    vsp::AddExcrescence( "Margin", vsp::EXCRESCENCE_MARGIN, excres_inputs[3] );
    results_id = vsp::ExecAnalysis("ParasiteDrag");
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

    dat_int = vsp::GetIntResults( results_id, "Num_Excres" );
    TEST_ASSERT( dat_int[0] == 4 );

    // Add Fifth Excrescence
    vsp::AddExcrescence( "Drag Area", vsp::EXCRESCENCE_DRAGAREA, excres_inputs[4] );
    results_id = vsp::ExecAnalysis("ParasiteDrag");
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

    dat_int = vsp::GetIntResults( results_id, "Num_Excres" );
    TEST_ASSERT( dat_int[0] == 5 );

    vector < double > dat_double = vsp::GetDoubleResults( results_id, "Excres_Input" );
    TEST_ASSERT( dat_double.size() == nexcres );
    for ( size_t i = 0; i < dat_double.size(); ++i )
    {
        TEST_ASSERT( dat_double[i] == excres_inputs[i] );
    }

    dat_double = vsp::GetDoubleResults( results_id, "Excres_Amount" );
    TEST_ASSERT( dat_double.size() == nexcres );
    for( size_t i = 0; i < dat_double.size(); ++i )
    {
        TEST_ASSERT_DELTA( dat_double[i], excres_amounts[i], excres_tol );
    }
}

void APITestSuiteParasiteDrag::TestSecondParasiteDragCalc()
{
    double tol = 0.00001;
    double second_res = 0.00491;

    // Execute
    printf("\tExecuting...\n");
    string results_id = vsp::ExecAnalysis("ParasiteDrag");
    printf("COMPLETE\n");
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

    vector < double > dat = vsp::GetDoubleResults( results_id, "Total_Cd_Total" );
    TEST_ASSERT( abs(second_res - dat[0]) < tol );

    vsp::PrintResults( results_id );                             // Get & Display Results

                                                                 // Final check for errors
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE
    printf("\n");
}

void APITestSuiteParasiteDrag::TestChangeOptions()
{
    vector < int > turbeqn(1);
    turbeqn[0] = vsp::CF_TURB_IMPLICIT_KARMAN_SCHOENHERR;
    vector < double > sref(1);
    sref[0] = 42.50;
    vector < double > vinf(1);
    vinf[0] = 630;
    vector < double > alt(1);
    alt[0] = 30000;

    string wing_geom = vsp::FindGeom( "MainWing", 0 );
    vsp::SetParmVal( wing_geom, "FFWingEqnType", "ParasiteDragProps", vsp::FF_W_DATCOM );
    TEST_ASSERT( vsp::GetParmVal(wing_geom, "FFWingEqnType", "ParasiteDragProps") );

    string pod_geom = vsp::FindGeom( "PodGeom", 0 );
    vsp::SetParmVal( pod_geom, "FFBodyEqnType", "ParasiteDragProps", vsp::FF_B_JENKINSON_FUSE );
    TEST_ASSERT( vsp::GetParmVal(pod_geom, "FFBodyEqnType", "ParasiteDragProps") );

    vsp::SetIntAnalysisInput( "ParasiteDrag", "TurbCfEqnChoice", turbeqn );
    vsp::SetDoubleAnalysisInput( "ParasiteDrag", "Sref", sref );
    vsp::SetDoubleAnalysisInput( "ParasiteDrag", "Vinf", vinf );
    vsp::SetDoubleAnalysisInput( "ParasiteDrag", "Altitude", alt );

    vsp::Update();
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

    double tol = 0.00001;
    double third_res = 0.01512;

    // Execute
    printf("\tExecuting...\n");
    string results_id = vsp::ExecAnalysis("ParasiteDrag");
    printf("COMPLETE\n");
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

    vector < double > dat = vsp::GetDoubleResults( results_id, "Total_Cd_Total" );
    TEST_ASSERT( abs(third_res - dat[0]) < tol );

    vsp::PrintResults( results_id );                             // Get & Display Results

                                                                 // Final check for errors
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE
    printf("\n");
}

void APITestSuiteParasiteDrag::TestRevertToSimpleModel()
{
    vsp::SetAnalysisInputDefaults( "ParasiteDrag" );

    printf("Deleting Count (10000*CD)\n");
    vsp::DeleteExcrescence( 0 ); // Count (10000*CD)
    string results_id = vsp::ExecAnalysis("ParasiteDrag");
    vector < int > dat_int = vsp::GetIntResults( results_id, "Num_Excres" );
    TEST_ASSERT( dat_int[0] == 4 );

    printf("Deleting CD\n");
    vsp::DeleteExcrescence( 0 ); // CD
    results_id = vsp::ExecAnalysis("ParasiteDrag");
    dat_int = vsp::GetIntResults( results_id, "Num_Excres" );
    TEST_ASSERT( dat_int[0] == 3 );

    printf("Deleting %% of CD_Geom\n");
    vsp::DeleteExcrescence( 0 ); // % of CD_Geom
    results_id = vsp::ExecAnalysis("ParasiteDrag");
    dat_int = vsp::GetIntResults( results_id, "Num_Excres" );
    TEST_ASSERT( dat_int[0] == 2 );

    printf("Deleting Margin\n");
    vsp::DeleteExcrescence( 0 ); // Margin
    results_id = vsp::ExecAnalysis("ParasiteDrag");
    dat_int = vsp::GetIntResults( results_id, "Num_Excres" );
    TEST_ASSERT( dat_int[0] == 1 );

    printf("Deleting Drag Area (D/q)\n");
    vsp::DeleteExcrescence( 0 ); // Drag Area (D/q)
    results_id = vsp::ExecAnalysis("ParasiteDrag");
    dat_int = vsp::GetIntResults( results_id, "Num_Excres" );
    TEST_ASSERT( dat_int[0] == 0 );

    TestFirstParasiteDragCalc();
}

void APITestSuiteParasiteDrag::TestS3VikingModel()
{
    vsp::VSPRenew();

    vsp::SetVSP3FileName( "S3VikingModel.vsp3" );  // this still needs to be done even if a call to WriteVSPFile is made
    vsp::Update();
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

    //==== Save Vehicle to File ====//
    vsp::WriteVSPFile( vsp::GetVSPFileName(), vsp::SET_ALL );
    TEST_ASSERT( !vsp::ErrorMgr.PopErrorAndPrint( stdout ) );    //PopErrorAndPrint returns TRUE if there is an error we want ASSERT to check that this is FALSE

    double test_tol = 0.0001;
    string wing = vsp::AddGeom("WING", "");                 // Add Wing
    vsp::SetGeomName(wing, "Wing");
    string fuse = vsp::AddGeom("FUSELAGE", "");             // Add Fuselage
    vsp::SetGeomName(fuse, "Fuse");
    string htail = vsp::AddGeom("WING", "");                // Add H-Tail
    vsp::SetGeomName(htail, "H-Tail");
    string vtail = vsp::AddGeom("WING", "");                // Add V-Tail
    vsp::SetGeomName(vtail, "V-Tail");

    // === Fuse ==== //
    // Editing of Fuse Sections
    vsp::SetParmVal(fuse, "Ellipse_Height", "XSecCurve_1", 4.5);
    vsp::SetParmVal(fuse, "Ellipse_Width", "XSecCurve_1", 6.0);

    vsp::InsertXSec(fuse, 1, vsp::XS_GENERAL_FUSE);
    vsp::SetParmVal(fuse, "Height", "XSecCurve_2", 8.0);
    vsp::SetParmVal(fuse, "TopStr", "XSecCurve_2", 0.05);
    vsp::SetParmVal(fuse, "BotStr", "XSecCurve_2", 1.83);
    vsp::SetParmVal(fuse, "UpStr", "XSecCurve_2", 1.83);
    vsp::SetParmVal(fuse, "LowStr", "XSecCurve_2", 1.5);

    vsp::CopyXSec(fuse, 2);
    vsp::PasteXSec(fuse, 3);
    vsp::PasteXSec(fuse, 4);

    // Placement of Fuse Sections
    vsp::SetParmVal(fuse, "ZLocPercent", "XSec_0", -0.05);

    vsp::SetParmVal(fuse, "XLocPercent", "XSec_1", 0.10);
    vsp::SetParmVal(fuse, "ZLocPercent", "XSec_1", -0.035);

    vsp::SetParmVal(fuse, "ZLocPercent", "XSec_2", 0.0);
    vsp::SetParmVal(fuse, "XLocPercent", "XSec_2", 0.20);

    vsp::SetParmVal(fuse, "XLocPercent", "XSec_3", 0.40);

    vsp::SetParmVal(fuse, "XLocPercent", "XSec_4", 0.55);

    vsp::SetParmVal(fuse, "ZLocPercent", "XSec_5", 0.03);

    vsp::SetParmVal(fuse, "Length", "Design", 49.4);

    // ==== Wing ==== //
    vsp::SetDriverGroup(wing, 1, vsp::SPAN_WSECT_DRIVER, vsp::ROOTC_WSECT_DRIVER, vsp::TIPC_WSECT_DRIVER);
    vsp::SetParmVal(wing, "X_Rel_Location", "XForm", 15.0);
    vsp::SetParmVal(wing, "Z_Rel_Location", "XForm", 3.0);
    vsp::SetParmVal(wing, "Y_Rel_Rotation", "XForm", 2.0);
    vsp::SetParmVal(wing, "Span", "XSec_1", 34.016);
    vsp::SetParmVal(wing, "Root_Chord", "XSec_1", 14.08);
    vsp::SetParmVal(wing, "Tip_Chord", "XSec_1", 3.5);
    vsp::SetParmVal(wing, "Sweep", "XSec_1", 15.0);
    vsp::SetParmVal(wing, "Sweep_Location", "XSec_1", 0.25);
    vsp::SetParmVal(wing, "ThickChord", "XSecCurve_0", 0.163);
    vsp::SetParmVal(wing, "ThickChord", "XSecCurve_1", 0.139);

    // ==== H-Tail ==== //
    vsp::SetParmVal(htail, "X_Rel_Location", "XForm", 40.0);
    vsp::SetParmVal(htail, "Z_Rel_Location", "XForm", 4.0);
    vsp::SetParmVal(htail, "Sweep", "XSec_1", 20.0);
    vsp::SetParmVal(htail, "Sweep_Location", "XSec_1", 0.25);
    vsp::SetParmVal(htail, "Dihedral", "XSec_1", 5);
    vsp::SetParmVal(htail, "Span", "XSec_1", 13.5);
    vsp::SetParmVal(htail, "Root_Chord", "XSec_1", 9.308);
    vsp::SetParmVal(htail, "Tip_Chord", "XSec_1", 4.025);
    vsp::SetParmVal(htail, "ThickChord", "XSecCurve_0", 0.10);

    // ==== V-Tail ==== //
    vsp::SetParmVal(vtail, "X_Rel_Location", "XForm", 35.0);
    vsp::SetParmVal(vtail, "Z_Rel_Location", "XForm", 4.0);
    vsp::SetParmVal(vtail, "X_Rel_Rotation", "XForm", 90.0);
    vsp::SetParmVal(vtail, "Sym_Planar_Flag", "Sym", 0);
    vsp::SetParmVal(vtail, "Span", "XSec_1", 13.6);
    vsp::SetParmVal(vtail, "Sweep", "XSec_1", 38.5);
    vsp::SetParmVal(vtail, "Sweep_Location", "XSec_1", 0.25);
    vsp::SetParmVal(vtail, "Root_Chord", "XSec_1", 13.208);
    vsp::SetParmVal(vtail, "Tip_Chord", "XSec_1", 4.917);

    vsp::Update();

    // ==== Set Parasite Drag Settings ==== //
    string PDcon = vsp::FindContainer("ParasiteDragSettings", 0);
    string groupname = "ParasiteDrag";

    vector <string> fileNameInput = vsp::GetStringAnalysisInput("ParasiteDrag", "FileName");
    fileNameInput[0] = "ParasiteDragExample";
    vsp::SetStringAnalysisInput("ParasiteDrag", "FileName", fileNameInput);

    vector <int> vinfUnitInput = vsp::GetIntAnalysisInput("ParasiteDrag", "VelocityUnit");
    vinfUnitInput[0] = vsp::V_UNIT_FT_S;
    vsp::SetIntAnalysisInput("ParasiteDrag", "VelocityUnit", vinfUnitInput);

    vector <double> vinfFCinput = vsp::GetDoubleAnalysisInput("ParasiteDrag", "Vinf");
    vinfFCinput[0] = 629;
    vsp::SetDoubleAnalysisInput("ParasiteDrag", "Vinf", vinfFCinput);

    vector <double> altInput = vsp::GetDoubleAnalysisInput("ParasiteDrag", "Altitude");
    altInput[0] = 40000;
    vsp::SetDoubleAnalysisInput("ParasiteDrag", "Altitude", altInput);

    vector <double> srefInput = vsp::GetDoubleAnalysisInput("ParasiteDrag", "Sref");
    srefInput[0] = 598.0;
    vsp::SetDoubleAnalysisInput("ParasiteDrag", "Sref", srefInput);

    vector <int> turbcfEqnChoiceInput = vsp::GetIntAnalysisInput("ParasiteDrag", "TurbCfEqnChoice");
    turbcfEqnChoiceInput[0] = vsp::CF_TURB_IMPLICIT_KARMAN_SCHOENHERR;
    vsp::SetIntAnalysisInput("ParasiteDrag", "TurbCfEqnChoice", turbcfEqnChoiceInput);

    string FFEqnChoice;
    vsp::SetParmVal(wing, "FFWingEqnType", "ParasiteDragProps", vsp::FF_W_DATCOM);
    vsp::SetParmVal(fuse, "FFBodyEqnType", "ParasiteDragProps", vsp::FF_B_SCHEMENSKY_FUSE);
    vsp::SetParmVal(htail, "FFWingEqnType", "ParasiteDragProps", vsp::FF_W_JENKINSON_TAIL);
    vsp::SetParmVal(vtail, "FFWingEqnType", "ParasiteDragProps", vsp::FF_W_JENKINSON_TAIL);

    string excresVal = vsp::FindParm(PDcon, "ExcresVal", groupname);
    string excresType = vsp::FindParm(PDcon, "ExcresType", groupname);

    vsp::AddExcrescence("Miscellaneous", vsp::EXCRESCENCE_COUNT, 8.5);
    vsp::AddExcrescence("Cowl Boattail", vsp::EXCRESCENCE_CD, 0.0003);

    vsp::AddExcrescence("Percentage Example", vsp::EXCRESCENCE_PERCENT_GEOM, 5);
    vsp::DeleteExcrescence(2);

    string ridpd = vsp::ExecAnalysis("ParasiteDrag");

    vector <double> dat = vsp::GetDoubleResults(ridpd, "Total_Cd_Total", 0);
    double tol = 0.018572*0.10;

    TEST_ASSERT_DELTA(dat[0], 0.018572, tol);
}

void APITestSuiteParasiteDrag::TestUSStandardAtmosphere1976()
{
    printf( "Starting US Standard Atmosphere 1976 Table Creation. \n" );
    vsp::WriteAtmosphereCSVFile( "USStandardAtmosphere1976Data.csv", vsp::ATMOS_TYPE_US_STANDARD_1976 );
    printf( "COMPLETE. \n" );
}

void APITestSuiteParasiteDrag::TestUSAF1966()
{
    printf( "Starting USAF Atmosphere 1966 Table Creation. \n" );
    vsp::WriteAtmosphereCSVFile( "USAFAtmosphere1966Data.csv", vsp::ATMOS_TYPE_HERRINGTON_1966 );
    printf( "COMPLETE. \n" );
}

void APITestSuiteParasiteDrag::TestFormFactorEquations()
{
    printf( "Starting Body Form Factor Data Creation. \n" );
    vsp::WriteBodyFFCSVFile( "BodyFormFactorData.csv" );
    printf( "COMPLETE. \n" );

    printf( "Starting Wing Form Factor Data Creation. \n" );
    vsp::WriteWingFFCSVFile( "WingFormFactorData.csv" );
    printf( "COMPLETE. \n" );
}

void APITestSuiteParasiteDrag::TestFrictionCoefficientEquations()
{
    printf( "Starting Turbulent Friciton Coefficient Data Creation. \n" );
    vsp::WriteTurbCfCSVFile( "FrictionCoefficientData.csv" );
    printf( "COMPLETE. \n" );
}