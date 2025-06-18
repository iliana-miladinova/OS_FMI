#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>

typedef struct {
    uint32_t uid;
    uint16_t s1;
    uint16_t s2;
    uint32_t start_time;
    uint32_t end_time;
} Info;

double findAvg(uint32_t* dur, size_t n) {
    double sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += dur[i];
    }
    double avg = sum / n;
    return avg;
}

double findDisp(uint32_t* dur, size_t n, double avg) {
    double disp_sum = 0;
    for (size_t i = 0; i < n; i++) {
        double diff = dur[i] - avg;
        disp_sum += diff * diff;
    }
    double disp = disp_sum / n;
    return disp;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errx(1, "Invalid num of args");
    }

    int fd1 = open(argv[1], O_RDONLY);
    if (fd1 < 0) {
        err(2, "Could not open file for reading");
    }

    struct stat st;
    if (fstat(fd1, &st) < 0) {
        err(3, "Could not fstat");
    }

    size_t file_size = st.st_size;
    size_t num_records = file_size / sizeof(Info);

    uint32_t duration[num_records];
    Info info;
    int bytes_count;
    uint32_t ids[num_records];

    size_t ind = 0;
    while ((bytes_count = read(fd1, &info, sizeof(info))) == sizeof(info)) {
        duration[ind] = info.end_time - info.start_time;
        ids[ind] = info.uid;
        ind++;
    }

    // find number of users
    uint32_t uniq_uids[num_records];
    size_t uniq_count = 0;
    for (size_t i = 0; i < num_records; i++) {
        int found = 0;
        for (size_t j = 0; j < uniq_count; j++) {
            if (ids[i] == uniq_uids[j]) {
                found = 1;
                break;
            }
        }

        if (!found) {
            uniq_uids[uniq_count++] = ids[i];
        }
    }

    // find avg
    double avg = findAvg(duration, ind);

    // find disp
    double disp = findDisp(duration, ind, avg);

    for (size_t i = 0; i < uniq_count; i++) {
        uint32_t uid = uniq_uids[i];
        uint32_t max_dur = 0;

        for (size_t j = 0; j < ind; j++) {
            if (ids[j] == uid && duration[j] > disp) {
                if (duration[j] > max_dur) {
                    max_dur = duration[j];
                }
            }
        }

        if (max_dur > 0) {
            if (write(1, &uid, sizeof(uid)) != sizeof(uid)) {
                err(4, "Could not write to stdout");
            }
            if (write(1, &max_dur, sizeof(max_dur)) != sizeof(max_dur)) {
                err(4, "Could not write to stdout");
            }
        }
    }

    close(fd1);
}
