#pragma once

#include <vector>
#include <QColor>
#include <QImage>

namespace Diginstrument
{
class ColorPalette
{
  public:
    static std::vector<QColor> generatePalette(unsigned int colors)
    {
        const double goldenRatio = 0.618033988749895;
        double hue = 0;
        std::vector<QColor> res;
        res.reserve(colors);
        for(int i =0; i<colors; i++)
        {
            if(i%2==0)
            {
                hue = goldenRatio * (180.0/(double)colors) * i;
            }
            else
            {
                hue = goldenRatio * ((180.0/(double)colors * i) + 180);
            }
            res.push_back(QColor::fromHsv((int)hue, 255, 255));
        }
        return res;
    }

    static std::vector<QImage> generatePaletteTextures(unsigned int amount)
    {
        const auto colors = generatePalette(amount);
        std::vector<QImage> res;
        res.reserve(colors.size());
        for(int i = 0; i<colors.size(); i++)
        {
            res.emplace_back(2, 2, QImage::Format_RGB32);
            res.back().fill(colors[i]);
        }
        return res;
    }
};
}