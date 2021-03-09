//
// Created by seledreams on 28/02/2021.
//

#ifndef LMMS_WAVTOOL_YAWU_H
#define LMMS_WAVTOOL_YAWU_H
class WAVTOOL_YAWU{
public:
    static int process(const std::string &outputPath,
                const std::string &inputPath,
                double scaledStartPoint,
                double scaledNoteLength,
                double p1,
                double p2,
                double p3,
                double v1,
                double v2,
                double v3,
                double v4,
                double overlap,
                double p4,
                double p5,
                double v5,
                bool p5_enabled = false);
};
#endif //LMMS_WAVTOOL_YAWU_H
