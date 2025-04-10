#include <lvgl.h>
#include <TFT_eSPI.h>
#include "ui.h"
#include "FT6236.h"
// SD card
#include <FS.h>
#include "SD.h"
#include "SPI.h"

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1;
static lv_color_t *buf2;

TFT_eSPI tft = TFT_eSPI();

#define I2C_SDA 21
#define I2C_SCL 22
// #define TP_RST 25
// #define TP_INT 21
// sd card
FT6236 touch = FT6236();

#if LV_USE_LOG != 0

void my_print(const char *buf)
{
  Serial.printf(buf);
  Serial.flush();
}
#endif

void setCS(uint8_t cs)
{
  // if (cs)
  //   digitalWrite(cs, HIGH);
  // else
  //   digitalWrite(cs, LOW);
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  // tft.startWrite();
  tft.pushImageDMA(area->x1, area->y1, w, h, (uint16_t *)color_p);
  // tft.endWrite();

  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  uint16_t touchX, touchY;
  if (touch.touched())
  {
    // Retrieve a point
    TS_Point p = touch.getPoint();
    touchX = p.x;
    touchY = p.y;
    data->state = LV_INDEV_STATE_PR;
    /*Set the coordinates*/
    data->point.x = touchX;
    data->point.y = touchY;
  }
  else
    data->state = LV_INDEV_STATE_REL;
}

#define SD_CS 15

static void sd_card_init(void)
{
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  // Make sure both devices are deactivated before starting SPI
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  delay(10);
  digitalWrite(SD_CS, LOW);
  // Initialize SPI first
  if (!SD.begin(4))
  {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized.");

  // Deactivate SD card and activate TFT
  digitalWrite(SD_CS, HIGH);
  delay(10);
  digitalWrite(TFT_CS, LOW); // Ready to use TFT
}

void sd_card_test_list_file(void)
{
  digitalWrite(SD_CS, LOW);
  lv_fs_dir_t dir;
  lv_fs_res_t res;
  setCS(false);
  res = lv_fs_dir_open(&dir, "A:/lvgl");
  setCS(false);
  if (res != LV_FS_RES_OK)
  {
    Serial.println("Failed to open directory");
    return;
  }

  char fn[256];
  while (1)
  {
    setCS(false);
    res = lv_fs_dir_read(&dir, fn);
    setCS(true);
    if (res != LV_FS_RES_OK)
    {
      Serial.println("Failed to read next file");
      break;
    }

    /*fn is empty, if not more files to read*/
    if (strlen(fn) == 0)
    {
      break;
    }

    printf("%s\n", fn);
  }

  setCS(false);
  lv_fs_dir_close(&dir);
  setCS(true);
}

void setup()
{
  Serial.begin(115200);

  if (!touch.begin(40, I2C_SDA, I2C_SCL)) // 40 in this case represents the sensitivity. Try higer or lower for better response.
  {
    Serial.println("Unable to start the capacitive touchscreen.");
  }
  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  Serial.println(LVGL_Arduino);
  Serial.println("I am LVGL_Arduino");
  sd_card_init();
  sd_card_test_list_file();
  lv_init();

#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print);
#endif
  pinMode(27, OUTPUT);
  digitalWrite(27, LOW);
  tft.begin();
  tft.setRotation(0);
  tft.initDMA();

  buf1 = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * TFT_WIDTH * 200, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL); // screenWidth * screenHeight/2
  buf2 = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * TFT_WIDTH * 200, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, TFT_WIDTH * 200);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);

  disp_drv.hor_res = TFT_WIDTH;
  disp_drv.ver_res = TFT_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  ui_init();

  Serial.println("Setup done");
  tft.startWrite();
}

void loop()
{
  // put your main code here, to run repeatedly:
  lv_timer_handler();
  delay(5);
}