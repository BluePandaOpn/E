#pragma once

#include <string>
#include <vector>

struct RemoteVersion {
    std::string zipName;
    std::string versionName;
    std::string downloadUrl;
};

class VersionManager {
public:
    static std::string cliVersion();
    std::vector<RemoteVersion> fetchRemoteVersions() const;
    int listRemoteVersions() const;
    int installVersion(const std::string& versionInput) const;
    int updateToLatest() const;
};
