<template>
  <v-container grid-list-lg>
    <v-layout text-xs-center>
      <v-flex xs12 sm6 offset-sm3>
        <v-card elevation="3">
          <v-img
            :src="require('../assets/logo.png')"
            contain
            height="200"
          ></v-img>
          <v-card-title primary-title>
            <div class="ma-auto">
              <span class="grey--text">IDF version: {{ version }}</span>
              <br />
              <span class="grey--text">ESP cores: {{ cores }}</span>
            </div>
          </v-card-title>
        </v-card>
      </v-flex>
    </v-layout>
    <v-layout text-xs-center wrap>
      <v-flex xs12 sm6 offset-sm3>
        <v-card elevation="3">
          <v-card-text>
            <v-container fluid grid-list-lg>
              <v-layout row wrap>
                <v-flex xs9>
                  <v-slider
                    v-model="M1"
                    :min="-100"
                    :max="100"
                    label="M1"
                  ></v-slider>
                </v-flex>
                <v-flex xs3>
                  <v-text-field
                    v-model="M1"
                    class="mt-0"
                    type="number"
                  ></v-text-field>
                </v-flex>
                <v-flex xs9>
                  <v-slider
                    v-model="M2"
                    :min="-100"
                    :max="100"
                    label="M2"
                  ></v-slider>
                </v-flex>
                <v-flex xs3>
                  <v-text-field
                    v-model="M2"
                    class="mt-0"
                    type="number"
                  ></v-text-field>
                </v-flex>
                <v-flex xs9>
                  <v-slider
                    v-model="M3"
                    :min="-100"
                    :max="100"
                    label="M3"
                  ></v-slider>
                </v-flex>
                <v-flex xs3>
                  <v-text-field
                    v-model="M3"
                    class="mt-0"
                    type="number"
                  ></v-text-field>
                </v-flex>
              </v-layout>
            </v-container>
          </v-card-text>
          <v-btn dark large color="red accent-4" @click="set_speed">
            <v-icon dark>check_box</v-icon>
          </v-btn>
        </v-card>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      version: null,
      cores: null,
      M1: 0,
      M2: 0,
      M3: 0,
    };
  },
  mounted() {
    this.$ajax
      .get("/api/v1/system/info")
      .then((data) => {
        this.version = data.data.version;
        this.cores = data.data.cores;
      })
      .catch((error) => {
        console.log(error);
      });
  },
  methods: {
    set_speed: function () {
      this.$ajax
        .post("/api/v1/motor/speed", {
          M1: this.M1,
          M2: this.M2,
          M3: this.M3,
        })
        .then((data) => {
          console.log(data);
        })
        .catch((error) => {
          console.log(error);
        });
    },
  },
};
</script>
