// from: http://stackoverflow.com/questions/2941491/compare-versions-as-strings
#ifndef VERSIONCOMPARE_H__
#define VERSIONCOMPARE_H__

#include <iostream>
#include <sstream>
#include <vector>
#include <iterator>

class Version
{
    // An internal utility structure just used to make the std::copy in the constructor easy to write.
    struct VersionDigit
    {
        int value;
        operator int() const {return value;}
    };
    friend std::istream& operator>>(std::istream& str, Version::VersionDigit& digit);
    public:
        Version(std::string const& versionStr)
        {
            // To Make processing easier in VersionDigit prepend a '.'
            std::stringstream   versionStream(std::string(".") + versionStr);

            // Copy all parts of the version number into the version Info vector.
            std::copy(  std::istream_iterator<VersionDigit>(versionStream),
                        std::istream_iterator<VersionDigit>(),
                        std::back_inserter(versionInfo)
                     );
        }

        // Test if two version numbers are the same.
        bool operator<(Version const& rhs) const
        {
            return std::lexicographical_compare(versionInfo.begin(), versionInfo.end(), rhs.versionInfo.begin(), rhs.versionInfo.end());
        }

    private:
        std::vector<int>    versionInfo;
};

// Read a single digit from the version.
std::istream& operator>>(std::istream& str, Version::VersionDigit& digit)
{
    str.get();
    str >> digit.value;
    return str;
}

#endif //VERSIONCOMPARE_H__
