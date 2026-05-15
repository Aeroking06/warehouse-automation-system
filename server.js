const express = require("express");
const mongoose = require("mongoose");

const app = express();
app.use(express.json());
app.use(express.static("public"));

// Connect to MongoDB
mongoose.connect("mongodb://127.0.0.1:27017/nfc_db")
  .then(() => console.log("MongoDB connected"))
  .catch(err => console.log(err));

// Schema (1 UID = 1 package)
const PackageSchema = new mongoose.Schema({
  uid: { type: String, unique: true }, // ensures no duplicates
  product: String,
  location: String,
  status: String,
  batch: String,
  source: String,
  time: { type: Date, default: Date.now }
});

const Package = mongoose.model("Package", PackageSchema);

// SAVE or UPDATE (based on UID)
app.post("/api/package", async (req, res) => {
  try {
    const { uid, product, location, status } = req.body;

    let existing = await Package.findOne({ uid });

    if (existing) {
      // Update existing package
      existing.status = status;
      existing.location = location;
      existing.time = new Date();
      await existing.save();
    } else {
      // 🆕 Create new package
      await Package.create({
        uid,
        product,
        location,
        status,
        batch: "B001",    
        source: "Factory A" 
      });
    }

    res.send("Saved");

  } catch (err) {
    console.log(err);
    res.status(500).send("Error");
  }
});

// GET ALL DATA (for dashboard)
app.get("/api/package", async (req, res) => {
  const data = await Package.find().sort({ time: -1 });
  res.json(data);
});

// RESET DATABASE
app.delete("/api/package", async (req, res) => {
  try {
    await Package.deleteMany({});
    res.send("Database cleared");
  } catch (err) {
    console.log(err);
    res.status(500).send("Error");
  }
});

app.listen(3000, () => console.log("Server running on port 3000"));