#include "util/compressivesensing.h"
#include "basic_sketch/basic_sketch.h"
#include "string.h"
#include <assert.h>
#include <basic_cm/tower.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))

// the arrays that defines the 1 to 4 pre-defined shapes of the tower
std::pair<int, int> tower_settings_1[3] = {std::make_pair(8, 2), std::make_pair(8, 2), std::make_pair(4, 1)};
std::pair<int, int> tower_settings_2[2] = {std::make_pair(4, 4), std::make_pair(8, 1)};
std::pair<int, int> tower_settings_3[3] = {std::make_pair(4, 2), std::make_pair(4, 2), std::make_pair(4, 1)};
std::pair<int, int> tower_settings_4[2] = {std::make_pair(8, 8), std::make_pair(16, 1)};

class basic_cm : public basic_sketch
{
protected:
  uint32_t w, d, ratio, zerothld;
  uint32_t **counters;
  Tower<uint32_t> **tres;
  std::pair<size_t, int *> **cres;
  int num_flags;

public:
  using basic_sketch::operator new;
  using basic_sketch::operator new[];
  using basic_sketch::operator delete;
  using basic_sketch::operator delete[];

  // construct the sketch with external parameter
  basic_cm(int argc, basic_sketch_string *argv)
  {

    w = argv[0].to_int();
    d = argv[1].to_int();
    ratio = argv[2].to_int();
    zerothld = argv[3].to_int();
    int shape = argv[4].to_int();

    counters = (uint32_t **)CALLOC(d, sizeof(uint32_t *));
    for (int i = 0; i < d; i++)
    {
      counters[i] = (uint32_t *)CALLOC(w, sizeof(uint32_t));
    }

    tres = (Tower<uint32_t> **)CALLOC(d, sizeof(Tower<uint32_t> *));
    cres =
        (std::pair<size_t, int *> **)CALLOC(d, sizeof(pair<size_t, int *> *));
    std::pair<int, int> *tower_settings_t;
    int levelcnt = 3;
    switch (shape)
    {
    case 1:

      tower_settings_t = tower_settings_1;

      break;
    case 2:

      tower_settings_t = tower_settings_2;
      levelcnt = 2;

      break;
    case 3:

      tower_settings_t = tower_settings_3;

      break;
    case 4:

      tower_settings_t = tower_settings_4;
      levelcnt = 2;

      break;
    }

    for (int i = 0; i < d; i++)
    {
      Tower<uint32_t> *temp =
          (Tower<uint32_t> *)CALLOC(1, sizeof(Tower<uint32_t>));
      temp->aux_setter(counters[i], w, levelcnt, tower_settings_t, false, false,
                       zerothld, -1);
      tres[i] = temp;
    }
  }

  // Constructing the sketch from the persistence data that Redis wrote
  basic_cm(const basic_sketch_string &s)
  {
    size_t tmp = 0;
    const char *ss = s.c_str();

    memcpy(&w, ss + tmp, sizeof(uint32_t));
    tmp += sizeof(uint32_t);

    memcpy(&d, ss + tmp, sizeof(uint32_t));
    tmp += sizeof(uint32_t);

    counters = (uint32_t **)CALLOC(d, sizeof(uint32_t *));
    for (int i = 0; i < d; i++)
    {
      counters[i] = (uint32_t *)CALLOC(w, sizeof(uint32_t));
      memcpy(counters[i], ss + tmp, w * sizeof(uint32_t));
      tmp += w * sizeof(uint32_t);
    }

    tres = (Tower<uint32_t> **)CALLOC(d, sizeof(Tower<uint32_t> *));
    for (int i = 0; i < d; i++)
    {
      tres[i] = (Tower<uint32_t> *)CALLOC(w, sizeof(Tower<uint32_t>));
      memcpy(tres[i], ss + tmp, w * sizeof(Tower<uint32_t>));

      tmp += w * sizeof(Tower<uint32_t>);
    }
    for (int i = 0; i < d; i++)
    {
      for (int j = 0; j < w; j++)
      {

        // READING std::pair<int, int> *tower_settings;
        for (int k = 0; k < tres[i][j].level_cnt; k++)
        {
          int *ptr_l;
          ptr_l = (int *)(ss + tmp);
          int first = *ptr_l;
          tmp += sizeof(int);

          int *ptr_r;
          ptr_r = (int *)(ss + tmp);
          int second = *ptr_r;
          tmp += sizeof(int);

          tres[i][j].tower_settings[k] = std::make_pair(first, second);
        }

        // READING uint32_t **tower_counters;
        for (int k = 0; k < tres[i][j].level_cnt; k++)
        {
          int bits = tres[i][j].tower_settings[k].first;

          int num_per_slot = 32 / bits;
          int packed_width = (tres[i][j].w + num_per_slot - 1) / num_per_slot;
          for (int p = 0; p < packed_width; p++)
          {
            uint32_t *ptr = (uint32_t *)(tmp + ss);
            tres[i][j].tower_counters[k][p] = *ptr;
            tmp += sizeof(uint32_t);
          }
        }

        // READING uint8_t **overflow_indicators;
        for (int k = 0; k < tres[i][j].level_cnt - 1; k++)
        {
          int o_packed_width = (tres[i][j].w + 7) / 8; // int counter_num = w;
          for (int p = 0; p < o_packed_width; p++)
          {
            unsigned char *ptr = (unsigned char *)(tmp + ss);
            tres[i][j].overflow_indicators[k][p] = *ptr;
            tmp += sizeof(unsigned char);
          }
        }

        // READING ShiftBF **sbfs;
        for (int k = 0; k < tres[i][j].level_cnt; k++)
        {
          ShiftBF **ptr = (ShiftBF **)(ss + tmp);
          ShiftBF *ptr_2 = ptr[k];
          ShiftBF content = ptr_2[0];
          tres[i][j].sbfs[k][0] = content;

          tmp += sizeof(ShiftBF);

          // READING the uint8_t *array inside the ShiftBF struct
          for (int p = 0; p < tres[i][j].sbf_size; p++)
          {
            for (int l = 0; l < tres[i][j].w + 10; l++)
            {
              uint8_t *ptr = (uint8_t *)(ss + tmp);
              tmp += sizeof(uint8_t);
              tres[i][j].sbfs[k]->array[l] = *ptr;
            }
          }
        }

        // READING the int *tower_widths
        for (int k = 0; k < tres[i][j].level_cnt; k++)
        {
          int *ptr = (int *)ss + tmp;
          tres[i][j].tower_widths[k] = *ptr;
          tmp += sizeof(int);
        }
      }
    }

    cres =
        (std::pair<size_t, int *> **)CALLOC(d, sizeof(pair<size_t, int *> *));
    for (int i = 0; i < d; i++)
    {
      cres[i] = (std::pair<size_t, int *> *)CALLOC(
          w, sizeof(std::pair<size_t, int *>));
      memcpy(cres[i], ss + tmp, w * sizeof(std::pair<size_t, int *>));
      tmp += w * sizeof(std::pair<size_t, int *>);
      for (int j = 0; j < w; j++)
      {

        int len = cres[i][j].first;
        for (int k = 0; k < len; k++)
        {
          int *ptr = (int *)(ss + tmp);
          cres[i][j].second[k] = *ptr;
          tmp += sizeof(int);
        }
      }
    }
  }
  ~basic_cm()
  {
    for (int i = 0; i < d; i++)
    {
      FREE(counters[i]);
      FREE(tres[i]);
      FREE(cres[i]);
    }
  }

  // the function that counts the whole required size
  unsigned long to_string_prerequisite()
  {
    unsigned long count_tmp = 0;
    for (int i = 0; i < d; i++)
    {
      count_tmp += w * sizeof(Tower<uint32_t>);
    }

    for (int i = 0; i < d; i++)
    {
      for (int j = 0; j < w; j++)
      {

        for (int k = 0; k < tres[i][j].level_cnt; k++)
        {

          count_tmp += sizeof(std::pair<int, int>);
        }

        for (int k = 0; k < tres[i][j].level_cnt; k++)
        {
          int bits = tres[i][j].tower_settings[k].first;

          int num_per_slot = 32 / bits;
          int packed_width = (tres[i][j].w + num_per_slot - 1) / num_per_slot;

          count_tmp += packed_width * sizeof(uint32_t);
        }

        for (int k = 0; k < tres[i][j].level_cnt - 1; k++)
        {
          int o_packed_width = (tres[i][j].w + 7) / 8; // counter_num = w;

          count_tmp += o_packed_width * o_packed_width * sizeof(unsigned char);
        }

        for (int k = 0; k < tres[i][j].level_cnt; k++)
        {

          count_tmp += sizeof(ShiftBF);
          for (int p = 0; p < tres[i][j].sbf_size; p++)
          {

            count_tmp += (tres[i][j].w + 10) * sizeof(uint8_t);
          }
        }

        for (int k = 0; k < tres[i][j].level_cnt; k++)
        {

          count_tmp += sizeof(int);
        }
      }
    }

    for (int i = 0; i < d; i++)
    {

      count_tmp += w * sizeof(std::pair<size_t, int *>);
      for (int j = 0; j < w; j++)
      {
        int len = cres[i][j].first;
        for (int k = 0; j < len; k++)
        {

          count_tmp += len * sizeof(int);
        }
      }
    }
    return count_tmp;
  }

  // persistent: add all data structures to one string and store it in disk
  basic_sketch_string *to_string()
  {
    char *s = (char *)CALLOC(2 + w * d, sizeof(uint32_t));

    char *s_alt = (char *)realloc(
        s, sizeof(s) +
               to_string_prerequisite()); // to_string_prerequisite():the
                                          // function that counts the whole size

    size_t tmp = 0;

    memcpy(s_alt + tmp, &w, sizeof(uint32_t));
    tmp += sizeof(uint32_t);

    memcpy(s_alt + tmp, &d, sizeof(uint32_t));
    tmp += sizeof(uint32_t);

    for (int i = 0; i < d; i++)
    {
      memcpy(s_alt + tmp, counters[i], w * sizeof(uint32_t));
      tmp += w * sizeof(uint32_t);
    }

    for (int i = 0; i < d; i++)
    {
      memcpy(s_alt + tmp, tres[i], w * sizeof(Tower<uint32_t>));
      tmp += w * sizeof(Tower<uint32_t>);
    }

    for (int i = 0; i < d; i++)
    {
      for (int j = 0; j < w; j++)
      {

        // SAVING std::pair<int, int> *tower_settings
        for (int k = 0; k < tres[i][j].level_cnt; k++)
        {
          int *tmp_data;
          *tmp_data = tres[i][j].tower_settings[k].first;
          memcpy(s_alt + tmp, tmp_data, sizeof(int));
          tmp += sizeof(int);
          *tmp_data = tres[i][j].tower_settings[k].second;
          memcpy(s_alt + tmp, tmp_data, sizeof(int));
          tmp += sizeof(int);
        }

        // SAVING uint32_t **tower_counters;
        for (int k = 0; k < tres[i][j].level_cnt; k++)
        {
          int bits = tres[i][j].tower_settings[k].first;

          int num_per_slot = 32 / bits;
          int packed_width = (tres[i][j].w + num_per_slot - 1) / num_per_slot;

          memcpy(s_alt + tmp, tres[i][j].tower_counters[k],
                 packed_width * sizeof(uint32_t));
          tmp += packed_width * sizeof(uint32_t);
        }

        // SAVING uint8_t **overflow_indicators;
        for (int k = 0; k < tres[i][j].level_cnt - 1; k++)
        {
          int o_packed_width = (tres[i][j].w + 7) / 8; // counter_num = w;
          memcpy(s_alt + tmp, tres[i][j].overflow_indicators[k],
                 o_packed_width * sizeof(unsigned char));
          tmp += o_packed_width * o_packed_width * sizeof(unsigned char);
        }

        // SAVING ShiftBF **sbfs;
        for (int k = 0; k < tres[i][j].level_cnt; k++)
        {
          memcpy(s_alt + tmp, tres[i][j].sbfs[k], sizeof(ShiftBF));
          tmp += sizeof(ShiftBF);
          // saving the uint8_t *array inside the ShiftBF struct
          for (int p = 0; p < tres[i][j].sbf_size; p++)
          {
            memcpy(s_alt + tmp, tres[i][j].sbfs[k]->array,
                   (tres[i][j].w + 10) * sizeof(uint8_t));
            tmp += (tres[i][j].w + 10) * sizeof(uint8_t);
          }
        }

        // SAVING the int *tower_widths
        for (int k = 0; k < tres[i][j].level_cnt; k++)
        {
          memcpy(s_alt + tmp, tres[i][j].tower_widths, sizeof(int));
          tmp += sizeof(int);
        }
      }
    }

    for (int i = 0; i < d; i++)
    {

      memcpy(s_alt + tmp, cres[i], w * sizeof(std::pair<size_t, int *>));
      tmp += w * sizeof(std::pair<size_t, int *>);

      for (int j = 0; j < w / 50; j++)
      {
        memcpy(s_alt + tmp, cres[i][j].second, cres[i][j].first * sizeof(int));
        tmp += cres[i][j].first * sizeof(int);
      }
    }

    basic_sketch_string *bs = new basic_sketch_string(s_alt, tmp);
    delete s;
    delete s_alt;
    return bs;
  }

  // compress the sketch the tower sensing strategy
  basic_sketch_reply *towerSensingCompress(const int &argc,
                                           const basic_sketch_string *argv)
  {
    basic_sketch_string ratio_string = argv[0];
    basic_sketch_string zerothid_string = argv[1];
    basic_sketch_string shape_string = argv[2];
    int ratio = ratio_string.to_int();
    int zerothld = zerothid_string.to_int();
    int shape = shape_string.to_int();

    tres = (Tower<uint32_t> **)CALLOC(d, sizeof(Tower<uint32_t> *)); //
    cres =
        (std::pair<size_t, int *> **)CALLOC(d, sizeof(pair<size_t, int *> *));
    std::pair<int, int> tower_settings[] = {};
    std::pair<int, int> *tower_settings_t;
    int levelcnt = 3;
    switch (shape)
    {
    case 1:

      tower_settings_t = tower_settings_1;

      break;
    case 2:

      tower_settings_t = tower_settings_2;
      levelcnt = 2;

      break;
    case 3:

      tower_settings_t = tower_settings_3;

      break;
    case 4:

      tower_settings_t = tower_settings_4;
      levelcnt = 2;

      break;
    }
    for (int i = 0; i < d; i++)
    {
      Tower<uint32_t> *temp =
          (Tower<uint32_t> *)CALLOC(1, sizeof(Tower<uint32_t>));
      temp->aux_setter(counters[i], w, levelcnt, tower_settings_t, false, false,
                       zerothld, -1);
      tres[i] = temp;
    }

    for (int i = 0; i < d; i++)
    {
      CompressiveSensing<int> cs(w, ratio, w / 50, zerothld, i * 123);
      // num_flags = cs.num_frags;
      cres[i] = cs.compress(counters[i]);
    }

    basic_sketch_reply *result = new basic_sketch_reply;
    result->push_back("OK");
    return result;
  }

  // compress the sketch with sketch sensing strategy
  basic_sketch_reply *sketchSensingCompress(const int &argc,
                                            const basic_sketch_string *argv)
  {
    basic_sketch_string ratio_string = argv[0];
    basic_sketch_string zerothid_string = argv[1];
    basic_sketch_string shape_string = argv[2];
    int ratio = ratio_string.to_int();
    int zerothld = zerothid_string.to_int();
    int shape = shape_string.to_int();

    for (int i = 0; i < d; i++)
    {
      CompressiveSensing<int> cs(w, ratio, w / 50, zerothld, i * 123);
      // num_flags = cs.num_frags;
      cres[i] = cs.compress(counters[i]);
    }

    basic_sketch_reply *result = new basic_sketch_reply;
    result->push_back("OK");
    return result;
  }

  // Decompress the sketch with tower sensing strategy
  basic_sketch_reply *towerSensingDecompress(const int &argc,
                                             const basic_sketch_string *argv)
  {
    basic_sketch_string ratio_string = argv[0];
    basic_sketch_string zerothid_string = argv[1];
    int ratio = ratio_string.to_int();
    int zerothld = zerothid_string.to_int();

    uint32_t *tmp1 = (uint32_t *)CALLOC(w, sizeof(uint32_t));
    uint32_t *tmp2 = (uint32_t *)CALLOC(w, sizeof(uint32_t));

    for (int i = 0; i < d; i++)
    {
      auto &t = tres[i];
      auto &c = cres[i];

      t->decompress(tmp1);

      CompressiveSensing<int> cs(w, ratio, w / 50, zerothld, i * 123);

      cs.decompress(tmp2, c);

      for (int j = 0; j < w; j++)
        if (tmp1[j])
          counters[i][j] = tmp1[j];
        else
          counters[i][j] = tmp2[j];
    }
    basic_sketch_reply *result = new basic_sketch_reply;
    result->push_back("OK");
    return result;
  }

  // Decompress the sketch with sketch sensing strategy
  basic_sketch_reply *sketchSensingDecompress(const int &argc,
                                              const basic_sketch_string *argv)
  {
    basic_sketch_string ratio_string = argv[0];
    basic_sketch_string zerothid_string = argv[1];
    int ratio = ratio_string.to_int();
    int zerothld = zerothid_string.to_int();

    uint32_t *tmp2 = (uint32_t *)CALLOC(w, sizeof(uint32_t));

    for (int i = 0; i < d; i++)
    {
      auto &c = cres[i];

      CompressiveSensing<int> cs(w, ratio, w / 50, zerothld, i * 123);

      cs.decompress(tmp2, c);

      for (int j = 0; j < w; j++)

        counters[i][j] = tmp2[j];
    }
    basic_sketch_reply *result = new basic_sketch_reply;
    result->push_back("OK");
    return result;
  }

  // Insert into the sketch
  basic_sketch_reply *insert(const int &argc, const basic_sketch_string *argv)
  {
    basic_sketch_reply *result = new basic_sketch_reply;
    for (int c = 0; c < argc; ++c)
      for (uint32_t i = 0; i < d; ++i)
      {
        uint32_t *now_counter =
            counters[i] + HASH(argv[c].c_str(), argv[c].len(), i) % w;
        if (*now_counter != UINT32_MAX)
          *now_counter += 1;
      }

    result->push_back("OK");
    return result;
  }

  // Query the sketch
  basic_sketch_reply *query(const int &argc, const basic_sketch_string *argv)
  {
    basic_sketch_reply *result = new basic_sketch_reply;
    for (int c = 0; c < argc; ++c)
    {
      uint32_t ans = UINT32_MAX;
      for (uint32_t i = 0; i < d; ++i)
      {
        ans =
            MIN(ans, counters[i][HASH(argv[c].c_str(), argv[c].len(), i) % w]);
      }
      result->push_back((long long)ans);
    }
    return result;
  }

  /****** add commands for redis ******/

  static basic_sketch_reply *Insert(void *o, const int &argc,
                                    const basic_sketch_string *argv)
  {
    return ((basic_cm *)o)->insert(argc, argv);
  }
  static basic_sketch_reply *
  TowerSensingCompress(void *o, const int &argc,
                       const basic_sketch_string *argv)
  {
    return ((basic_cm *)o)->towerSensingCompress(argc, argv);
  }
  static basic_sketch_reply *
  SketchSensingCompress(void *o, const int &argc,
                        const basic_sketch_string *argv)
  {
    return ((basic_cm *)o)->sketchSensingCompress(argc, argv);
  }
  static basic_sketch_reply *
  TowerSensingDecompress(void *o, const int &argc,
                         const basic_sketch_string *argv)
  {
    return ((basic_cm *)o)->towerSensingDecompress(argc, argv);
  }
  static basic_sketch_reply *
  SketchSensingDecompress(void *o, const int &argc,
                          const basic_sketch_string *argv)
  {
    return ((basic_cm *)o)->sketchSensingDecompress(argc, argv);
  }
  static basic_sketch_reply *Query(void *o, const int &argc,
                                   const basic_sketch_string *argv)
  {
    return ((basic_cm *)o)->query(argc, argv);
  }

  static int command_num() { return 6; }
  static basic_sketch_string command_name(int index)
  {
    basic_sketch_string tmp[] = {"insert",
                                 "query",
                                 "sketchSensingCompress",
                                 "sketchSensingDecompress",
                                 "towerSensingCompress",
                                 "towerSensingDecompress"};
    return tmp[index];
  }
  static basic_sketch_func command(int index)
  {
    basic_sketch_func tmp[] = {(basic_cm::Insert),
                               (basic_cm::Query),
                               (basic_cm::SketchSensingCompress),
                               (basic_cm::SketchSensingDecompress),
                               (basic_cm::TowerSensingCompress),
                               (basic_cm::TowerSensingDecompress)};
    return tmp[index];
  }
  static basic_sketch_string class_name() { return "basic_cm"; }
  static int command_type(int index)
  {
    int tmp[] = {0, 1, 1, 1, 1, 1};
    return tmp[index];
  }
  static char *type_name() { return "BASIC_CMS"; }
};