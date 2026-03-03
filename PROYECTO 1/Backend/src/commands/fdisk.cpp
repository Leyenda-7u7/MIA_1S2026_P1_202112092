#include "commands/fdisk.hpp"
#include "Structures.hpp"

#include <fstream>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <vector>

static bool readMBR(const std::string& path, MBR& mbr, std::string& err) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        err = "Error: no se pudo abrir el disco.";
        return false;
    }
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    if (!file) {
        err = "Error: no se pudo leer el MBR.";
        return false;
    }
    return true;
}

static bool writeMBR(const std::string& path, const MBR& mbr, std::string& err) {
    std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) {
        err = "Error: no se pudo abrir el disco para escritura.";
        return false;
    }
    file.seekp(0, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&mbr), sizeof(MBR));
    if (!file) {
        err = "Error: no se pudo escribir el MBR.";
        return false;
    }
    return true;
}

static long long toBytes(int32_t size, const std::string& unitStr) {
    if (size <= 0) return -1;

    std::string u = unitStr;
    if (u.empty()) u = "k";

    std::transform(u.begin(), u.end(), u.begin(), ::tolower);

    if (u == "k") return (long long)size * 1024LL;
    if (u == "m") return (long long)size * 1024LL * 1024LL;

    return -1;
}

static char normalizeType(const std::string& typeStr) {
    if (typeStr.empty()) return 'P';

    std::string t = typeStr;
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);

    if (t == "p") return 'P';
    if (t == "e") return 'E';
    if (t == "l") return 'L';

    return '\0';
}

static char normalizeFit(const std::string& fitStr) {
    if (fitStr.empty()) return 'W';

    std::string f = fitStr;
    std::transform(f.begin(), f.end(), f.begin(), ::tolower);

    if (f == "bf") return 'B';
    if (f == "ff") return 'F';
    if (f == "wf") return 'W';

    return '\0';
}

namespace cmd {

bool fdiskCreate(int32_t size,
                 const std::string& unitStr,
                 const std::string& path,
                 const std::string& typeStr,
                 const std::string& fitStr,
                 const std::string& name,
                 std::string& outMsg) {

    if (size <= 0 || path.empty() || name.empty()) {
        outMsg = "Error: parámetros inválidos para fdisk.";
        return false;
    }

    if (!std::filesystem::exists(path)) {
        outMsg = "Error: el disco no existe.";
        return false;
    }

    long long bytes = toBytes(size, unitStr);
    if (bytes <= 0) {
        outMsg = "Error: tamaño inválido.";
        return false;
    }

    char type = normalizeType(typeStr);
    if (type == '\0') {
        outMsg = "Error: tipo inválido (use P, E o L).";
        return false;
    }

    char fit = normalizeFit(fitStr);
    if (fit == '\0') {
        outMsg = "Error: fit inválido (BF, FF, WF).";
        return false;
    }

    MBR mbr{};
    std::string err;

    if (!readMBR(path, mbr, err)) {
        outMsg = err;
        return false;
    }

    // 🔹 Validar nombre duplicado
    for (int i = 0; i < 4; i++) {
        std::string existing(mbr.mbr_partitions[i].part_name);
        if (existing == name) {
            outMsg = "Error: ya existe una partición con ese nombre.";
            return false;
        }
    }

    // 🔹 Validar solo una extendida
    if (type == 'E') {
        for (int i = 0; i < 4; i++) {
            if (mbr.mbr_partitions[i].part_type == 'E' &&
                mbr.mbr_partitions[i].part_s > 0) {
                outMsg = "Error: ya existe una partición extendida.";
                return false;
            }
        }
    }

    // 🔹 Buscar slot libre
    int freeIndex = -1;
    for (int i = 0; i < 4; i++) {
        if (mbr.mbr_partitions[i].part_s == 0) {
            freeIndex = i;
            break;
        }
    }

    if (freeIndex == -1) {
        outMsg = "Error: no hay espacio para más particiones.";
        return false;
    }

    // 🔹 Calcular inicio real ordenando por posición
    int32_t start = sizeof(MBR);

    std::vector<std::pair<int32_t,int32_t>> used;

    for (int i = 0; i < 4; i++) {
        auto& p = mbr.mbr_partitions[i];
        if (p.part_s > 0) {
            used.push_back({p.part_start, p.part_s});
        }
    }

    std::sort(used.begin(), used.end());

    for (auto& seg : used) {
        if (start + bytes <= seg.first) {
            break;
        }
        start = seg.first + seg.second;
    }

    if ((start + bytes) > mbr.mbr_tamano) {
        outMsg = "Error: no hay espacio suficiente en el disco.";
        return false;
    }

    auto& part = mbr.mbr_partitions[freeIndex];

    part.part_status = '0';
    part.part_type = type;
    part.part_fit = fit;
    part.part_start = start;
    part.part_s = (int32_t)bytes;
    part.part_correlative = -1;

    std::memset(part.part_name, 0, sizeof(part.part_name));
    std::strncpy(part.part_name, name.c_str(), sizeof(part.part_name)-1);

    std::memset(part.part_id, 0, sizeof(part.part_id));

    if (!writeMBR(path, mbr, err)) {
        outMsg = err;
        return false;
    }

    outMsg = "Partición creada correctamente: " + name;
    return true;
}

} // namespace cmd