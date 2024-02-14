#include <algorithm>
#include <cwctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <locale>
#include <sstream>
#include <vector>

class HexDataFormatter {
  public:
    HexDataFormatter( const std::wstring &input ) : input_( input ) {}

    std::wstring formatAndPrint() {
        std::wstring formattedOutput = formatHexPairs( input_ );
        std::wstring result = pairDigitsWithNewline( formattedOutput );
        std::wstring results = formatDigits( result );
        std::wstring hexData = SwapPositions( results ); 
        // parseHexaString(hexData);
        return parseHexaString( hexData );
    }

  private:
    std::wstring input_;

    std::wstring formatHexPairs( const std::wstring &input ) {
        std::wostringstream formattedOutput;
        std::wistringstream iss( input );
        std::wstring line;

        while ( std::getline( iss, line ) ) {
            // Extract pairs of hexadecimal digits
            std::wistringstream hexStream(
                line.substr( line.find( L":" ) + 2 ) );
            std::wstring hexPair;

            while ( hexStream >> hexPair ) {
                // Output the processed pair with two spaces between each pair
                formattedOutput << hexPair << L" ";
            }

            formattedOutput << L'\n';
        }

        return formattedOutput.str();
    }

    // Function to pair digits in the given string with a newline after every
    // 8th pair
    std::wstring pairDigitsWithNewline( const std::wstring &input ) {
        std::wstringstream output;
        std::wistringstream iss( input );

        std::vector<std::wstring> tokens;
        std::copy( std::istream_iterator<std::wstring, wchar_t>( iss ),
                   std::istream_iterator<std::wstring, wchar_t>(),
                   std::back_inserter( tokens ) );

        for ( size_t i = 0; i < tokens.size(); i += 2 ) {
            output << tokens[i] << tokens[i + 1] << L" ";

            if ( ( i / 2 + 1 ) % 8 == 0 ) {
                output << L"\n";
            }
        }

        return output.str();
    }

    std::wstring formatDigits( const std::wstring &input ) {
        std::wstringstream output;
        std::wistringstream iss( input );

        std::vector<std::wstring> tokens;
        std::copy( std::istream_iterator<std::wstring, wchar_t>( iss ),
                   std::istream_iterator<std::wstring, wchar_t>(),
                   std::back_inserter( tokens ) );

        for ( size_t i = 0; i < tokens.size(); ++i ) {
            output << tokens[i] << L" ";

            if ( ( i + 1 ) % 4 == 0 ) {
                output << L"\n";
            }
        }

        return output.str();
    }

    std::wstring SwapPositions( const std::wstring &input ) {
        std::wistringstream iss( input );
        std::wostringstream formattedOutput;

        std::wstring line;
        while ( std::getline( iss, line ) ) {
            std::wistringstream lineStream( line );
            std::wostringstream formattedRow;

            std::wstring token;
            while ( lineStream >> token ) {
                // Swap positions of the first and last two characters
                if ( token.size() >= 4 ) {
                    std::swap( token[0], token[token.size() - 2] );
                    std::swap( token[1], token[token.size() - 1] );
                }
                formattedRow << token << L' ';
            }

            formattedOutput
                << formattedRow.str().substr( 0, formattedRow.str().size() - 1 )
                << L'\n';
        }

        return formattedOutput.str();
    }

    std::wstring formatDecimalValues( const std::wstring &line ) {
        std::wstring hexValues;
        for ( wchar_t c : line ) {
            if ( std::iswxdigit(
                     c ) ) { // Check if the character is a hexadecimal digit
                hexValues += c;
            }
        }

        std::wstringstream formattedValues;
        for ( size_t i = 0; i < hexValues.size(); i += 4 ) {
            int decimalValue =
                std::stoi( hexValues.substr( i, 4 ), nullptr, 16 );
            formattedValues << std::setw( 4 ) << std::setfill( L'0' )
                            << decimalValue << L" ";
        }

        return formattedValues.str();
    }

    std::wstring extractLines( const std::wstring &hexData ) {
        std::wistringstream iss( hexData );
        std::vector<std::wstring> lines;
        std::wstring line;

        while ( std::getline( iss, line ) ) {
            lines.push_back( line );
        }

        std::vector<std::wstring> result_lines;
        int index_last_1081 = -1;

        for ( int i = static_cast<int>( lines.size() ) - 1; i >= 0; --i ) {
            if ( lines[i].find( L"1081" ) == 0 ) {
                index_last_1081 = i;
                break;
            }
        }

        if ( index_last_1081 != -1 ) {
            for ( int i = index_last_1081; i < static_cast<int>( lines.size() );
                  ++i ) {
                result_lines.push_back( lines[i] );
            }
        }

        std::wstringstream result;
        for ( const auto &result_line : result_lines ) {
            result << result_line << L'\n';
        }

        return result.str();
    }

    std::wstring parseHexaString( const std::wstring &hexaData ) {
        std::wstring values;
        auto lines = extractLines( hexaData );
        std::wistringstream iss( lines );
        std::wstring line;
        int currentPressPoint = 0;
        int linesToPrint = 0;

        while ( std::getline( iss, line ) ) {
            line.erase( std::remove_if( line.begin(), line.end(), ::iswspace ),
                        line.end() );

            if ( line.find( L"1081" ) == 0 ) {
                std::wstring dateHex = line.substr( 4, 15 );
                int startYear =
                    2000 + std::stoi( dateHex.substr( 2, 2 ), nullptr, 16 );
                int startMonth =
                    std::stoi( dateHex.substr( 0, 2 ), nullptr, 16 );
                int startDay = std::stoi( dateHex.substr( 4, 2 ), nullptr, 16 );
                std::wstring dateStr = std::to_wstring( startYear ) + L"-" +
                                       std::to_wstring( startMonth ) + L"-" +
                                       std::to_wstring( startDay );
                std::wcout << L"\nDate: " << dateStr
                           << L"\nTest Phase: Final\n";
                std::wcout << L"Press Point: " << ++currentPressPoint << L"\n";
                linesToPrint = 2;
            } else if ( line.find( L"1082" ) == 0 ||
                        line.find( L"1083" ) == 0 ||
                        line.find( L"1084" ) == 0 ||
                        line.find( L"1085" ) == 0 ||
                        line.find( L"1086" ) == 0 ) {
                currentPressPoint = line[3] - L'0';
                std::wcout << L"Press Point: " << currentPressPoint << L"\n";
                linesToPrint = 2;
            } else if ( linesToPrint > 0 ) {
                std::wstring values = formatDecimalValues( line );
                std::wcout << values << L"\n";
                linesToPrint -= 1;
            }
        }

        return values;
    }
};