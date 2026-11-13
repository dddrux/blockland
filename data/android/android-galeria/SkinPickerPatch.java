// Dentro da classe (extends SDLActivity). Imports: android.content.Intent, android.net.Uri, java.io.*
public void blPickSkin() {
    runOnUiThread(new Runnable() { public void run() {
        Intent i = new Intent(Intent.ACTION_GET_CONTENT);
        i.setType("image/*"); i.addCategory(Intent.CATEGORY_OPENABLE);
        startActivityForResult(Intent.createChooser(i, "Escolher skin"), 7731);
    }});
}
@Override protected void onActivityResult(int req, int res, Intent data) {
    super.onActivityResult(req, res, data);
    if (req != 7731 || res != RESULT_OK || data == null || data.getData() == null) return;
    try {
        InputStream in = getContentResolver().openInputStream(data.getData());
        File out = new File(getExternalFilesDir(null), "skin_pick.png.tmp");
        OutputStream os = new FileOutputStream(out); byte[] b = new byte[16384]; int n;
        while ((n = in.read(b)) > 0) os.write(b, 0, n);
        os.close(); in.close();
        out.renameTo(new File(getExternalFilesDir(null), "skin_pick.png"));
    } catch (Exception e) { }
}
