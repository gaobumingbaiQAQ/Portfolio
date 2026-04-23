using UnityEngine;
using System.Collections;

public class GameController : MonoBehaviour
{
    public static GameController Instance;

    [Header("UI设置")]
    public GameObject startPanel; // 开始画面 (黑色背景)
    public GameObject endPanel;   // 结束画面

    [Header("游戏设置")]
    public int totalClues = 4;//四个线索
    public float winDelayTime = 10f;//十秒延迟胜利结算

    private int currentClues = 0;//记数

    [HideInInspector]
    public bool isGamePlaying = false;

    // 用于显示的临时文字
    private string displayString = "";

    void Awake()
    {
        Instance = this;
    }

    void Start()
    {
        // 游戏刚开始
        if (startPanel != null) startPanel.SetActive(true);
        if (endPanel != null) endPanel.SetActive(false);

        isGamePlaying = false;
        Time.timeScale = 0;

        // 初始化文字
        displayString = "线索收集：0 / " + totalClues;
    }

    void Update()
    {
        // 按任意键开始
        if (!isGamePlaying && startPanel.activeSelf)
        {
            if (Input.anyKeyDown)
            {
                StartGame();
            }
        }
    }

    void OnGUI()
    {
        // 只有游戏进行中才显示
        if (isGamePlaying)
        {
            // 1. 设置字体样式 (白色，超大，加粗)
            GUIStyle myStyle = new GUIStyle();
            myStyle.fontSize = 40;            // 字号40
            myStyle.normal.textColor = Color.white; 
            myStyle.fontStyle = FontStyle.Bold;   // 加粗

            // 2. 在屏幕左上角画字
            GUI.Label(new Rect(20, 20, 400, 100), displayString, myStyle);
        }
    }

    public void StartGame()
    {
        isGamePlaying = true;
        Time.timeScale = 1;
        startPanel.SetActive(false);
        // 不需要再去 setActive scoreText 了
    }

    public void AddClue()
    {
        currentClues++;

        // 更新要画在屏幕上的那句话
        displayString = "线索收集：" + currentClues + " / " + totalClues;

        if (currentClues >= totalClues)
        {
            displayString = "线索集齐！数据上传中...";
            StartCoroutine(WaitAndWin());
        }
    }

    IEnumerator WaitAndWin()
    {
        yield return new WaitForSeconds(winDelayTime);
        WinGame();
    }

    public void WinGame()
    {
        isGamePlaying = false; // 这会让左上角的字自动消失 (因为OnGUI里判断了)

        if (endPanel != null) endPanel.SetActive(true);
        Time.timeScale = 0;
        Cursor.lockState = CursorLockMode.None;
        Cursor.visible = true;
    }
}