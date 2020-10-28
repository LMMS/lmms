#pragma once

#include <nlohmann/json.hpp>
#include "SplineSpectrum.hpp"
#include "Dimension.h"

using json = nlohmann::json;

namespace Diginstrument
{
class JSONConverter
{
  public:
    static json toJSON(const SplineSpectrum<double, 4> & spline)
    {
        json res;

        for(auto & c : spline.getLabels())
        {
            res[c.first] = c.second;
        }

        auto splineCopy = spline;
        json pieces = json::array();
        for(auto & piece : splineCopy.getSpline().getPieces())
        {
            json pieceJSON;
            pieceJSON["control_points"] = piece.getSpline().getControlPoints();
            pieceJSON["knot_vector"] = piece.getSpline().getKnotVector();
            pieces.push_back(pieceJSON);
        }
        res["pieces"] = std::move(pieces);
        return res;
    }

    static json toJSON(const PartialSet<double> & partialSet)
    {
        json res;
        for(auto & c : partialSet.getLabels())
        {
            res[c.first] = c.second;
        }

        res["sample_rate"] = partialSet.getSampleRate();

        res["partials"] = json::array();
        for(const auto & p : partialSet.get())
        {
            json partialObject;
            vector<double> phases, amps;
            partialObject["frequency"] = p.front().frequency;
            phases.reserve(p.size());
            amps.reserve(p.size());
            for(const auto & c : p)
            {
                phases.push_back(c.phase);
                amps.push_back(c.amplitude);
            }
            partialObject["phases"] = phases;
            partialObject["magnitudes"] = amps;
            res["partials"].push_back(std::move(partialObject));
        }
        return res;
    }

    static json toJSON(const Diginstrument::Dimension & dimension)
    {
        json res;
        res["label"] = dimension.name;
        res["min"] = dimension.min;
        res["max"] = dimension.max;
        res["default"] = dimension.defaultValue;
        res["shifting"] = dimension.shifting;
        return res;
    }

    static json toJSON(
        std::string name,
        const std::vector<Diginstrument::Dimension> & dimensions,
        const std::vector<SplineSpectrum<double, 4>> & spectra,
        const std::vector<PartialSet<double>> & partials)
    {
        //TODO: "coordinates" not included, as they are not used anywhere anyway
        json res;

        res["spectra"] = json::array();
        for(const auto & s : spectra)
        {
           res["spectra"].push_back(toJSON(s));
        }

        res["dimensions"] = json::array();
        for(const auto & d : dimensions)
        {
           res["dimensions"].push_back(toJSON(d));
        }

        res["partial_sets"] = json::array();
        for(const auto & p : partials)
        {
           res["partial_sets"].push_back(toJSON(p));
        }

        res["spectrum_type"] = "partials";
        res["name"] = name;

        return res;
    }

    static SplineSpectrum<double, 4> splineFromJSON(json object)
    {
        PiecewiseBSpline<double, 4> piecewise;
        vector<pair<string, double>> labels;
        for(auto & p : object["pieces"])
        {
            BSpline<double, 4> spline;
            spline.setControlPoints(p["control_points"]);
            spline.setKnotVector(p["knot_vector"]);
            piecewise.add(spline);
        }
        for(auto & e : object.items())
        {
            if(e.value().is_number()) labels.emplace_back(e.key(), e.value());
        }
        return SplineSpectrum<double, 4>(std::move(piecewise), std::move(labels));
    }

    static PartialSet<double> partialSetFromJSON(json object)
    {
        vector<vector<Diginstrument::Component<double>>> partials;
        vector<pair<string, double>> labels;
        for(auto & p : object["partials"])
        {
            std::vector<Diginstrument::Component<double>> partial;
            partial.reserve(p["magnitudes"].size());
            for(int i = 0; i<p["magnitudes"].size(); i++)
            {
                partial.emplace_back(p["frequency"], p["phases"][i], p["magnitudes"][i]);
            }
            partials.push_back(std::move(partial));
        }
        for(auto & e : object.items())
        {
            if(e.value().is_number() && e.key()!="sample_rate") labels.emplace_back(e.key(), e.value());
        }
        return PartialSet<double>(std::move(partials), std::move(labels), object["sample_rate"]);
    }

    static Diginstrument::Dimension dimensionFromJSON(json object)
    {
        if(!object["default"].is_null()) return Diginstrument::Dimension(object["label"], object["min"], object["max"], object["shifting"]);
        else return Diginstrument::Dimension(object["label"], object["min"], object["max"], object["shifting"], object["default"]);
    }
    
};
};