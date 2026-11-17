import fs from "node:fs/promises";
import { FileBlob, SpreadsheetFile } from "@oai/artifact-tool";

const source = "C:/Fschool_IoT/Tai_lieu_ky_thuat_cam_bien_IoT_Backend_v2.xlsx";
const renderDir = "C:/Fschool_IoT/.codex_tmp_analysis/workbook_renders";
await fs.mkdir(renderDir, { recursive: true });

const input = await FileBlob.load(source);
const workbook = await SpreadsheetFile.importXlsx(input);
const overview = await workbook.inspect({
  kind: "workbook,sheet,table,region,drawing",
  maxChars: 30000,
  tableMaxRows: 50,
  tableMaxCols: 20,
  tableMaxCellChars: 300,
});
console.log("---OVERVIEW---");
console.log(overview.ndjson);

console.log("---SHEETS---");
for (let i = 0; i < workbook.worksheets.items.length; i++) {
  const sheet = workbook.worksheets.getItemAt(i);
  const used = sheet.getUsedRange();
  console.log(JSON.stringify({ index: i, name: sheet.name, address: used?.address ?? null }));
  const region = await workbook.inspect({
    kind: "region,formula,drawing",
    sheetId: sheet.name,
    range: used?.address ?? "A1:Z100",
    maxChars: 30000,
    tableMaxRows: 120,
    tableMaxCols: 30,
    tableMaxCellChars: 500,
    options: { maxResults: 500 },
  });
  console.log(region.ndjson);

  const image = await workbook.render({
    sheetName: sheet.name,
    autoCrop: "all",
    scale: 1.5,
    format: "png",
  });
  const safe = sheet.name.replace(/[^a-zA-Z0-9_-]+/g, "_");
  await fs.writeFile(`${renderDir}/${String(i + 1).padStart(2, "0")}_${safe}.png`, new Uint8Array(await image.arrayBuffer()));
}
