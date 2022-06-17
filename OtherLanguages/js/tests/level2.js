const Lerc = require("../dist/LercDecode.js");
const fs = require("fs");

const datadir = __dirname.split("/").slice(0, -4).join("/") + "/data/lerc";

function formatPixelBlock(pb) {
  const pixels = pb.pixels.map((band) => band.join(","));
  const mask = pb.mask?.join(",");
  const statistics = pb.statistics;
  if (statistics?.length) {
    statistics.forEach((bandStat) => {
      const { depthStats } = bandStat;
      if (depthStats) {
        depthStats.minValues = depthStats.minValues.join(",");
        depthStats.maxValues = depthStats.maxValues.join(",");
      }
    });
  }
  const validPixelCountFromMask = pb.mask ? pb.mask.reduce((a, b) => a + b) : pb.width * pb.height;
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

function log(pass, message) {
  if (pass) {
    console.log("\x1b[32m%s\x1b[0m", `$PASS ${message}`);
  } else {
    console.error("\x1b[41m%s\x1b[0m", `$FAIL ${message}`);
  }
}

function runTest(lercFileName, compare = false) {
  const ndim = fs.readFileSync(datadir + "/" + lercFileName);
  let result;
  try {
    result = Lerc.decode(ndim);
  } catch {
    log(false, lercFileName);
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
    log(pass, lercFileName);
    return pass;
  }
  return true;
}

function level2() {
  const listFileName = datadir + "/" + "list.txt";
  if (!fs.existsSync(listFileName)) {
    log(false, "missing lerc file list.txt in datadir");
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
    log(true, `$SUMMARY ${passCount} / ${lercfileNameList.length} passed`);
    const failCount = lercfileNameList.length - passCount;
    if (failCount) {
      log(false, `$SUMMARY ${failCount} / ${lercfileNameList.length} failed`);
    }
  });
}
level2();
