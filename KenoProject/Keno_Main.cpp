/**
@file       Keno_Main.cpp
@brief      Implementation of Keno Project
@mainpage
@author     Mark L. Short
@date       October 1, 2014

  The casino game 'KENO' involves the selection of 20 balls from 80 balls numbered 
  1 ... 80. The player selects k numbers (1-20).  The payoff depends on how many 
  of the player's numbers that the casino machine selects.  For example, if the 
  player selects 9 numbers and 5 of them are generated by the machine, he/she wins 
  $4.00; if 6 are generated, the player wins $43.00.

  This program is to determine the probability of each possible situation and place 
  these probabilites in a 20 x 21 array of real values.  The entries in the [ith] 
  row assume that the player has selected i numbers.  The entry in the [jth] column 
  is the probability that the player catches j spots out of i possible.  Of course, 
  if j > i, the probability is 0.0.  In general, if i <= j, the probability is given 
  by:

      P (catch j out of i numbers) = C ( i, j ) * P1 * P2 / P3, where
      C (i, j) = i! / ((i-j)! * j!)

      P1 ( factors ) = 20 * 19 * 18 ... factors, where factors = j
      P2 ( factors ) = 60 * 59 * 58 ... factors, where factors = i - j
      P3 ( factors ) = 80 * 79 * 78 ... factors, where factors = i

  Together with this program specification there is a sheet of payoffs for between 1 & 9
  spots marked.  Calculate for each number of spots marked the "expected value" of a $1
  bet.
*/

/** @defgroup KenoProject   Keno Project
*/

#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include "DebugUtility.h"


typedef unsigned __int64 QWORD;    //< 64 bit unsigned integer  (0 to 18,446,744,073,709,551,615)



QWORD    calcFactorial       (WORD wN);
double   calcPartialFactorial(WORD wN, WORD wNumTerms);
DWORD    calcCombinations    (DWORD dwN, DWORD dwR);
double   calcKenoProbability (DWORD dwNumMarked, DWORD dwCatch);


constexpr const int g_MAX_ROWS             = 20;   //< used to set array bounds where the index = '(number of player 'marked' balls) - 1'
constexpr const int g_MAX_COLS             = 21;   //< used to set array bounds where the index = to catch size
constexpr const int g_TOTAL_BALLS          = 80;   //< This is the total of balls (1..80) in the simulation
constexpr const int g_MAX_SELECTABLE_BALLS = 20;   //< This is the maximum number of player selectable balls allowed.

/// Relative output path
constexpr const TCHAR g_szOutputDataPath[] = _T("\\Data\\");
/// Save the values in "Keno.xlsx"
constexpr const TCHAR g_szFileName[] = _T("Keno.xlsx");

/// The entries in the [ith] row assumes that the player has 'marked' i numbers
/// The entry in the [jth] column is the probability that the player catches j spots out of i possible
double g_rgProbability[g_MAX_ROWS][g_MAX_COLS] = { 0.0 }; //< array for calculated probability matrix

constexpr const int g_MAX_PAYOUT_ROWS  = 9; //< corresponds to 'spot(s) marked + 1'
constexpr const int g_MAX_PAYOUT_COLS  = 9; //< corresponds to 'number of balls caught + 1'
constexpr const int g_MAX_SPOTS_MARKED = 9;
                               
constexpr const double g_rgCatchPayOut[g_MAX_PAYOUT_ROWS][g_MAX_PAYOUT_COLS] = 
//Catch  1     2     3     4       5       6       7        8        9
    { { 3.0,  0.0,  0.0,  0.0,    0.0,    0.0,    0.0,     0.0,     0.0 },   // 1 Spot marked
      { 0.0, 12.0,  0.0,  0.0,    0.0,    0.0,    0.0,     0.0,     0.0 },   // 2 Spots marked
      { 0.0,  1.0, 42.0,  0.0,    0.0,    0.0,    0.0,     0.0,     0.0 },   // 3 Spots marked
      { 0.0,  1.0,  3.0, 120.0,   0.0,    0.0,    0.0,     0.0,     0.0 },   // 4 Spots marked
      { 0.0,  0.0,  1.0,   9.0, 800.0,    0.0,    0.0,     0.0,     0.0 },   // 5 Spots marked
      { 0.0,  0.0,  1.0,   4.0,  88.0, 1500.0,    0.0,     0.0,     0.0 },   // 6 Spots marked
      { 0.0,  0.0,  0.0,   2.0,  20.0,  350.0,  700.0,     0.0,     0.0 },   // 7 Spots marked
      { 0.0,  0.0,  0.0,   0.0,   9.0,   90.0, 1500.0, 20000.0,     0.0 },   // 8 Spots marked
      { 0.0,  0.0,  0.0,   0.0,   4.0,   43.0, 3000.0,  4000.0, 25000.0 } }; // 9 Spots marked

/**
@sa http://en.wikipedia.org/wiki/Expected_value
*/
double g_rgExpectedValue[g_MAX_SPOTS_MARKED] = { 0.0 };


/**
  @brief calcFactorial

  A recursive factorial implimentation

  @param [in] wN       value used for factorial operation

  @retval QWORD         containing computed results

  @note the return result can get very big and possibly overflow, 
        there are not checks for this
*/
QWORD calcFactorial (WORD wN)
{
    if ( wN > 1 )
        return wN * calcFactorial ( wN - 1 );
    else
        return 1;
}

/**
  @brief calcPartialFactorial 
 
  This performs a factorial computation of 'wN' utilizing only a 
  'wNumTerms' number of the highest valued terms of in traditional 
  factorial computation.

  For example, calcPartialFactorial(10, 4) would initiate a factorial
  computation, but would stop after evaluating the top 4 terms as follows:
    ( 10 * 9 * 7 * 6 )

  If 'wNumTerms' == 0, then a '1' is immediately returned.

  @param [in] wN                initial term
  @param [in] wNumTerms         number of terms used in partial factorial

  @retval double                containing the calculated partial factorial
*/
double calcPartialFactorial (WORD wN, WORD wNumTerms)
{
#ifdef _DEBUG
    DebugTrace (_T ("   %s: N[%d] NumTerms[%3d]"), __FUNCTIONW__, wN, wNumTerms);
#endif _DEBUG

    double fResult = 1.0;

    for ( WORD i = 0; i < wNumTerms; i++ )
    {
        fResult = fResult * (wN - i);
    }

#ifdef _DEBUG
    DebugTrace (_T ("= %f \n"), fResult);
#endif _DEBUG

    return fResult;
}

/**
  @brief calcCombinations - "N things taken R at a time, without repetition"

  Combination is the quantity of subgroups of a size 'R' that can be formed 
  out of a group of a size 'N' in which the order is NOT important.  For example
  given 3 fruits (an apple, an orange and a pear), there are 3 combinations of 2
  that can be drawn from this set: {apple, pear}, {apple, orange}, {pear, orange}.
  This expression is often written mathematically as C (N, R) where R is less than 
  or equal to N, calculated as N! / R!(N-R)! and which is 0 when R > N.
 
  @param [in] dwN           Group Size - number of things to choose from
  @param [in] dwR           Subgroup Size - number of things chosen

  @retval DWORD  containing the number of 'dwR' sized subgroups that can be 
                 formed from a set containing 'dwN' number of elements.

  @sa http://en.wikipedia.org/wiki/Combination
*/
DWORD calcCombinations (DWORD dwN, DWORD dwR)
{ 
#ifdef _DEBUG
    DebugTrace (_T ("   %s: N[%d] R[%d]"), __FUNCTIONW__, dwN, dwR);
#endif

    DWORD dwResult = 0;

    if ( dwR <= dwN )
    {
        const QWORD qwNFactorial   = calcFactorial ( static_cast<WORD>(dwN) );
        const QWORD qwRFactorial   = calcFactorial ( static_cast<WORD>(dwR) );

        const QWORD qwDifFactorial = calcFactorial ( static_cast<WORD>(dwN - dwR) );

        dwResult = static_cast<DWORD>(qwNFactorial / (qwRFactorial * qwDifFactorial));
    }

#if _DEBUG
    DebugTrace (_T ("= %u \n"), dwResult);
#endif

    return dwResult;
}

/**
  @brief calcKenoProbability

  Calulates the probability of a 'dwCaught' sized catch from any set of 
  'dwNumMarked' number of player picked balls, based on a total of 80 KENO balls
  with the maximum number of balls selectable being 20.

  @param [in] dwNumMarked       The number of KENO ball spots a player has 'marked' or 
                                selected 
  @param [in] dwCaught          The catch size of interest which probability is calculated 
                                against

  @retval double         containing the calculated probability of a matching a subset of 
                         potential 'dwCaught' sized number of balls from a set consisting 
                         of 'dwNumMarked' number of spots from the possible 20 selectable balls.
*/
double  calcKenoProbability ( DWORD dwNumMarked, DWORD dwCaught )
{
#ifdef _DEBUG
    DebugTrace (_T ("%s: NumMarked[%d] Caught[%d] \n"), __FUNCTIONW__, dwNumMarked, dwCaught);
#endif _DEBUG

    double fResult = 0.0;

    const DWORD dwNumCombinations = calcCombinations (dwNumMarked, dwCaught);
    const double qwP1 = calcPartialFactorial ( static_cast<WORD>(g_MAX_SELECTABLE_BALLS), static_cast<WORD>(dwCaught) );
    const double qwP2 = calcPartialFactorial ( static_cast<WORD>(g_TOTAL_BALLS - g_MAX_SELECTABLE_BALLS), static_cast<WORD>(dwNumMarked - dwCaught) );
    const double qwP3 = calcPartialFactorial ( static_cast<WORD>(g_TOTAL_BALLS), static_cast<WORD>(dwNumMarked) );

// the actual formula given was C(N, R) * P1 * P2 / P3
    fResult = static_cast<double>(dwNumCombinations) * qwP1 * qwP2 / qwP3;

#ifdef _DEBUG
    DebugTrace (_T ("= %.20f \n"), fResult);
#endif _DEBUG

    return fResult;
}


#pragma region import_block

// regarding the #import directive
// see http://msdn.microsoft.com/EN-US/library/298h7faa(v=VS.120,d=hv.2).aspx
// see https://msdn.microsoft.com/en-us/library/8etzzkb6(v=vs.140).aspx
 
// Microsoft Object Object Library
//#pragma message ("Make sure the path to MSO DLL is correct.")
//#import "C:\\Program Files\\Common Files\\Microsoft Shared\\OFFICE15\\MSO.DLL" raw_interfaces_only rename ("RGB", "MSORGB") rename("DocumentProperties", "MSODocumentProperties")
//#import "C:\\Program Files\\Microsoft Office\\root\\VFS\\ProgramFilesCommonX86\\Microsoft Shared\\Office16\\MSO.DLL" raw_interfaces_only no_namespace rename ("RGB", "MSORGB") rename("DocumentProperties", "MSODocumentProperties")

//#pragma message ("Make sure the path to EXCEL EXE is correct.")
//#import "C:\\Program Files\\Microsoft Office 15\\root\\office15\\EXCEL.EXE" rename ("RGB", "ExcelRGB") rename ("DialogBox", "ExcelDialogBox") rename ("CopyFile", "ExcelCopyFile") rename ("ReplaceText", "ExcelReplaceText") inject_statement("struct _VBProjectPtr;") inject_statement("struct VBEPtr;")
//#import "C:\\Program Files\\Microsoft Office\\root\\Office16\\EXCEL.EXE" rename ("RGB", "ExcelRGB") rename ("DialogBox", "ExcelDialogBox") rename ("CopyFile", "ExcelCopyFile") rename ("ReplaceText", "ExcelReplaceText") inject_statement("struct _VBProjectPtr;") inject_statement("struct VBEPtr;")

// import MSO.DLL using it's TypeLibID 
#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52" \
    raw_interfaces_only rename ("RGB", "MSORGB") \
    rename("DocumentProperties", "MSODocumentProperties")

// import Excel using it's ProgID
#import "progid:Excel.Workspace" \
    rename("RGB", "ExcelRGB") \
    rename("DialogBox", "ExcelDialogBox") \
    rename("CopyFile", "ExcelCopyFile") \
    rename("ReplaceText", "ExcelReplaceText") \
    inject_statement("struct _VBProjectPtr;") \
    inject_statement("struct VBEPtr;") \
    no_auto_exclude


#pragma endregion import_block

/**
  @brief Exports Keno Probability data to and Excel spreadsheet.

  @param [in] pWorkBook       A reference to an existing Excel Workbook Object

  @retval int
*/
int ExportKenoProbabilityDataSheet (Excel::_WorkbookPtr& pWorkBook)
{
    // add a new worksheet

    Excel::_WorksheetPtr pSheet = pWorkBook->ActiveSheet;

    _bstr_t bstrSheetName (_T("Keno Probability Matrix"));
    pSheet->put_Name ( bstrSheetName.GetBSTR() );

    Excel::RangePtr pRange = pSheet->GetCells ( );

    const TCHAR* szRowFmt = _T ("%d Spots(s) Marked");
    const TCHAR* szColFmt = _T ("%d Ball(s) Caught");
    TCHAR szRowHeader[32] = { 0 };
    TCHAR szColHeader[32] = { 0 };

    // format Row labels
    for ( int i = 0; i < g_MAX_ROWS; i++ )
    {
        _sntprintf (szRowHeader, _countof (szRowHeader) - 1, szRowFmt, i + 1);
        pRange->Item[i + 2][1] = szRowHeader;
    }

    // format Column labels
    for ( int j = 0; j < g_MAX_COLS; j++ )
    {
        _sntprintf (szColHeader, _countof (szColHeader) - 1, szColFmt, j);
        pRange->Item[1][j + 2] = szColHeader;
    }

    // Fill the worksheet
    for ( int i = 0; i < g_MAX_ROWS; i++ )
    {
        for ( int j = 0; j < g_MAX_COLS; j++ )
        {
            pRange->Item[i + 2][j + 2] = g_rgProbability[i][j];
        }
    }

    // set the cell format
    pRange->NumberFormat = _T ("0.??????????");
    Excel::RangePtr pColumns = pRange->GetColumns ( );

    // have Excel auto-size columns based on size of data
    pColumns->AutoFit ( );

    // underline the the column labels
    Excel::RangePtr   pHdrRange = pSheet->Range[_T ("B1:V1")][vtMissing];
    Excel::BordersPtr pBorders  = pHdrRange->GetBorders ( );
    Excel::BorderPtr  pBorder   = pBorders->GetItem (Excel::xlEdgeBottom);
    pBorder->PutLineStyle (Excel::xlContinuous);

    return 0;
}

/**
  @brief Exports Expected Value data to an Excel spreadsheet

  @param [in] pWorkBook       A reference to an existing Excel Workbook Object

  @retval int
*/
int ExportKenoPayOutDataSheet (Excel::_WorkbookPtr& pWorkBook)
{
    // add a new worksheet
    Excel::_WorksheetPtr pSheet = pWorkBook->Worksheets->Add ( );

    _bstr_t bstrSheetName (_T("Expected 'Pay Out' Values"));
    pSheet->put_Name ( bstrSheetName.GetBSTR () );

    Excel::RangePtr pRange = pSheet->GetCells ( );

    const TCHAR* szRowFmt = _T ("%d Spots(s) Marked");
    const TCHAR* szColHdr = _T ("Expected Value");
    TCHAR szRowHeader[32] = { 0 };

    for ( int i = 0; i < g_MAX_SPOTS_MARKED; i++ )
    {
        _sntprintf (szRowHeader, _countof (szRowHeader) - 1, szRowFmt, i + 1);
        pRange->Item[i + 2][1] = szRowHeader;
    }


    pRange->Item[1][2] = szColHdr;


    for ( int i = 0; i < g_MAX_SPOTS_MARKED; i++ )
    {

        pRange->Item[i + 2][2] = g_rgExpectedValue[i];
    }

    // set the cell format
    pRange->NumberFormat = _T ("0.??????????");
    Excel::RangePtr pColumns = pRange->GetColumns ( );

    // have Excel auto-size columns based on size of data
    pColumns->AutoFit ( );

    // underline the the column headers
    Excel::RangePtr   pHdrRange = pSheet->Range[_T ("B1")][vtMissing];
    Excel::BordersPtr pBorders  = pHdrRange->GetBorders ( );
    Excel::BorderPtr  pBorder   = pBorders->GetItem (Excel::xlEdgeBottom);
    pBorder->PutLineStyle (Excel::xlContinuous);

    return 0;
}

/**
  @brief ExportDataToExcel

  This method creates an instance of an Excel Component Object Model (COM) object
  and proceeds to export the computed data to an Excel spreadsheet.

  @retval int
*/
int ExportDataToExcel(void)
{
    // Create Excel Application Object pointer    
    Excel::_ApplicationPtr pXL;

    if ( FAILED (pXL.CreateInstance ("Excel.Application")) )
    {
#ifdef _DEBUG
        dbg << "Failed to initialize Excel::_Application!" << std::endl;
#endif
        return 0;
    }

    // Get the Workbooks collection
    Excel::WorkbooksPtr pBooks = pXL->Workbooks;
    Excel::_WorkbookPtr  pBook = pBooks->Add ((long) Excel::xlWorksheet);
 
    // Switch off alert prompting to save as   
    pXL->put_DisplayAlerts (LOCALE_USER_DEFAULT, VARIANT_FALSE);

    // Let us 1st try to get the current executable path
    TCHAR szDir[_MAX_PATH] = {0};

    if (GetModulePath(szDir, _countof(szDir) - 1 ) != nullptr)
    {
        size_t nLoc = _tcslen(szDir);

        // check to make sure not dealing with empty string
        if (nLoc)
        {  // get rid of trailing backslash if it exists
            if (szDir[nLoc - 1] == _T('\\'))
                szDir[nLoc - 1] = _T('\0');

            // effectively get the parent directory
            TCHAR* pLastBackslash = _tcsrchr(szDir, _T('\\')); 
            if (pLastBackslash)
                *pLastBackslash = _T('\0');

            _tcsncat(szDir, g_szOutputDataPath, min(_countof(g_szOutputDataPath), _MAX_PATH - _tcslen(szDir)) );
        }
    }

    pXL->PutDefaultFilePath (LOCALE_USER_DEFAULT, _bstr_t(szDir) );

    ExportKenoProbabilityDataSheet(pBook);
    ExportKenoPayOutDataSheet     (pBook);

    Excel::_WorksheetPtr pSheet = pXL->ActiveSheet;

    const size_t nLen = _tcslen(szDir);
    // check for errors and to make sure we have room to concatenate
    if ( (nLen != 0) && (nLen + _countof(g_szFileName) < _MAX_PATH) ) 
    {        
        _tcsncat(szDir, g_szFileName, _countof(g_szFileName) );
        pSheet->SaveAs (g_szFileName);
    }
    else
    {
        pSheet->SaveAs (_T("C:\\Temp\\Keno.xlsx"));
    }
    
    pBooks->Close();

    // And switch back on again...  
    pXL->put_DisplayAlerts (LOCALE_USER_DEFAULT, VARIANT_TRUE);

    pXL->Quit ();

    return 0;
}




int _tmain(int argc, _TCHAR* argv[])
{

#ifdef _DEBUG

    dbg.open(_T("KenoProject_dbg.txt"));

    dbg << std::setprecision (10);

    dbg << "Calculating Keno Probabilites Value(s)"        << std::endl;
    dbg << "---------------------------------------------" << std::endl;

#endif

    // Calculate the array of Keno probabilities

    for ( int i = 0; i < g_MAX_ROWS; i++ )      // i + 1 = '(number of spots 'marked')'
    {
        const int iNumSpotsMarked = i + 1;  // this is for readability
        for ( int j = 0; j < g_MAX_COLS; j++ )  // j = balls caught 
        {
            if ( iNumSpotsMarked >= j )
            { 
#ifdef _DEBUG
                dbg << "Calculating probability of a catch of [" << j << "] balls out of [" << iNumSpotsMarked << "] 'marked' numbers = ";
#endif
                // Probability of 'j' Ball(s) caught from set of 'i+1' player 'marked' spots or numbers
                g_rgProbability[i][j] = calcKenoProbability ( iNumSpotsMarked, j );
#ifdef _DEBUG
                dbg << g_rgProbability[i][j] << std::endl;
#endif
            }
            else
            {
                // probability is zero
            }
        }
    }

#ifdef _DEBUG

    dbg << "Calculating Expected Value(s)" << std::endl;
    dbg << "---------------------------------------------" << std::endl;

#endif

    // Calculate the array of expected values for a $1 bet

    /*************************************************************************

    The expected value of a discrete random variable is the probability-weighted 
    average of all possible values. In other words, each possible value the random 
    variable can assume is multiplied by its probability of occurring, and the resulting 
    products are summed to produce the expected value. The same works for continuous random 
    variables, except the sum is replaced by an integral and the probabilities by probability 
    densities. The formal definition subsumes both of these and also works for distributions 
    which are neither discrete nor continuous: the expected value of a random variable is the 
    integral of the random variable with respect to its probability measure.

    @cite http://en.wikipedia.org/wiki/Expected_value

    GIVEN: "Together with this program specification there is a sheet of payoffs 
            for between 1 & 9 spots marked.  Calculate for each number of spots 
            marked the 'expected value' of a $1 bet."

            - The KENO probability of "C" ball(s) getting caught out of "M" spots
              marked will be denoted as 'KP(M, C)'.
            - The payout of "C" ball(s) getting caught out of "M" spots marked will
              be denoted as 'PO(M, C)'.

            The expected $1 value of 9 spots marked should be equal to:
                KP(9, 9) * PO(9, 9) * 1/10 + 
                KP(9, 8) * PO(9, 8) * 1/10 +
                KP(9, 7) * PO(9, 7) * 1/10 +
                       ...          * 1/10 +
                KP(9, 0) * PO(9, 0) * 1/10
    */
    for ( int i = 0; i < g_MAX_SPOTS_MARKED; i++ )        // i + 1 = 'number of spots marked'
    {
        for ( int j = 0; j < g_MAX_SPOTS_MARKED; j++ )    // j + 1 = 'number of balls caught'
        {
            double dPayout = g_rgCatchPayOut[i][j];       // PO(M, C)
            if ( dPayout > 0 )
            {
                // lets lookup the associated probability -  KP(M, C)
                const double dKenoProb     = g_rgProbability[i][j+1];

                g_rgExpectedValue[i] += (dKenoProb * dPayout / (i+2));   // we have to divide by i+2 here because we need to 
                                                                         // account for i is zero-based and we need to account
                                                                         // for the number of terms to be averaged is actually,
                                                                         // M+1.
#ifdef _DEBUG                
                dbg << "KP(" << i+1 << "," << j+1 << ") =" << dKenoProb << std::endl;
                dbg << "PO(" << i+1 << "," << j+1 << ") =" << dPayout   << std::endl;

                dbg << "Expected Value of [" << i + 1 << "] spots marked " << g_rgExpectedValue[i] << std::endl;
#endif
            }
        }
    }



    // Initialize the COM libraries needed to interface with Excel
    HRESULT hr = ::CoInitializeEx (nullptr, COINIT_MULTITHREADED);

    if ( FAILED (hr) )
    {
#ifdef _DEBUG
        dbg << "Failed to initialize COM library. Error code = 0x"
            << std::hex << hr << std::endl;
#endif
        return hr;
    }

    ExportDataToExcel( );

    // Release COM resources
    ::CoUninitialize ( );


    return 0;
}

