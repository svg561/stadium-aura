# Stadium Aura Mic Matching Engine

The engine is relative: a conservative source-family compensation is blended first, then a target-family contour is blended. It does not identify or clone a specific microphone.

Order: source compensation -> broad problem-zone control -> target character -> air/body protection -> gain compensation.

Hardware Safe reduces maximum boosts, nonlinear density, and dynamic attenuation. Source and target changes are smoothed. The broad-zone tamer reacts to signal energy and avoids narrow, ringing cuts.

`Analyze Source` persistence is TODO. Until a broad-tone profile can be learned, saved, recalled, and tested honestly, the UI does not show a pretend analysis button.
