const Lerc = require("./dist/LercDecode.js");
const fs = require("fs");

const datadir = __dirname.split("/").slice(0, -3).join("/") + "/data/lerc";

function formatPixelBlock(pb) {
  const pixels = pb.pixels.map((band) => band.join(","));
  const mask = pb.mask?.join(",");
  const statistics = pb.statistics;
  if (statistics?.length) {
    statistics.forEach((bandStat) => {
      const { dimStats } = bandStat;
      if (dimStats) {
        dimStats.minValues = dimStats.minValues.join(",");
        dimStats.maxValues = dimStats.maxValues.join(",");
      }
    });
  }
  const validPixelCountFromMask = pb.mask ? pb.mask.reduce((a, b) => a + b) : null;
  const validPixelCountPerBand = pb.bandMasks
    ? pb.bandMasks.map((mask) => mask.reduce((a, b) => a + b)).join(",")
    : null;
  const bandMasks = pb.bandMasks ? pb.bandMasks.map((mask) => mask.join(",")) : pb.bandMasks;
  // push properties to the end
  delete pb.pixels;
  delete pb.mask;
  delete pb.bandMasks;
  return { ...pb, validPixelCountFromMask, validPixelCountPerBand, pixels, mask, bandMasks };
}

function runTest(lercFileName, compare = false) {
  const ndim = fs.readFileSync(datadir + "/" + lercFileName);
  let result;
  try {
    result = Lerc.decode(ndim);
  } catch {
    console.error(`$FAIL ${lercFileName}`);
    return false;
  }
  const actual = formatPixelBlock(result);
  const outputFilePath = datadir + "/test/" + lercFileName + ".json";
  const baseFilePath = datadir + "/base/" + lercFileName + ".json";
  fs.writeFileSync(outputFilePath, JSON.stringify(actual), "utf-8");
  // compare
  if (compare) {
    const base = JSON.parse(fs.readFileSync(baseFilePath, { encoding: "utf-8" }));
    const keys = ["width", "height", "pixelType", "statistics", "pixels", "dimCount", "bandMasks"];
    let diff = "";
    keys.forEach((key) => {
      if (JSON.stringify(base[key]) !== JSON.stringify(actual[key])) {
        diff += key + " ";
      }
    });
    const pass = diff === "";
    if (pass) {
      console.log(`$PASS ${lercFileName}`);
    } else {
      console.error(`$FAIL ${lercFileName}`);
    }
    return pass;
  }
  return true;
}

function level2() {
  const listFileName = datadir + "/" + "list.txt";
  if (!fs.existsSync(listFileName)) {
    console.log("missing lerc file list.txt in datadir");
    return;
  }
  Lerc.load().then(() => {
    let passCount = 0;
    const lercfileNameList = fs
      .readFileSync(listFileName, { encoding: "utf-8" })
      .split("\n")
      .filter((name) => name.includes("lerc"));
    lercfileNameList.forEach((lercfileName) => {
      if (runTest(lercfileName, true)) {
        passCount++;
      }
    });
    console.log(`$SUMMARY ${passCount} / ${lercfileNameList.length} passed`);
  });
}
level2();
